// Copyright 2008 Dolphin Emulator Project / 2017 Citra Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "core/core_timing.h"

#include <algorithm>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "common/assert.h"
#include "common/thread.h"
#include "common/threadsafe_queue.h"
#include "core/core.h"
#include "core/core_cpu.h"
#include "core/core_timing_util.h"

namespace CoreTiming {

static s64 global_timer;
static int slice_length;
static int next_slice_length;
static std::array<int, Core::NUM_CPU_CORES> downcount;

struct EventType {
    TimedCallback callback;
    const std::string* name;
};

struct Event {
    s64 time;
    u64 fifo_order;
    u64 userdata;
    const EventType* type;
};

// Sort by time, unless the times are the same, in which case sort by the order added to the queue
static bool operator>(const Event& left, const Event& right) {
    return std::tie(left.time, left.fifo_order) > std::tie(right.time, right.fifo_order);
}

static bool operator<(const Event& left, const Event& right) {
    return std::tie(left.time, left.fifo_order) < std::tie(right.time, right.fifo_order);
}

// unordered_map stores each element separately as a linked list node so pointers to elements
// remain stable regardless of rehashes/resizing.
static std::unordered_map<std::string, EventType> event_types;

// The queue is a min-heap using std::make_heap/push_heap/pop_heap.
// We don't use std::priority_queue because we need to be able to serialize, unserialize and
// erase arbitrary events (RemoveEvent()) regardless of the queue order. These aren't accomodated
// by the standard adaptor class.
static std::vector<Event> event_queue;
static u64 event_fifo_id;
// the queue for storing the events from other threads threadsafe until they will be added
// to the event_queue by the emu thread
static Common::MPSCQueue<Event, false> ts_queue;

constexpr int MAX_SLICE_LENGTH = 20000;

static std::mutex mutex;

// Are we in a function that has been called from Advance()
// If events are sheduled from a function that gets called from Advance(),
// don't change slice_length and downcount.
static bool is_global_timer_sane;

static EventType* ev_lost = nullptr;

static void EmptyTimedCallback(u64 userdata, s64 cyclesLate) {}

EventType* RegisterEvent(const std::string& name, TimedCallback callback) {
    // check for existing type with same name.
    // we want event type names to remain unique so that we can use them for serialization.
    ASSERT_MSG(event_types.find(name) == event_types.end(),
               "CoreTiming Event \"{}\" is already registered. Events should only be registered "
               "during Init to avoid breaking save states.",
               name.c_str());

    auto info = event_types.emplace(name, EventType{callback, nullptr});
    EventType* event_type = &info.first->second;
    event_type->name = &info.first->first;
    return event_type;
}

void UnregisterAllEvents() {
    ASSERT_MSG(event_queue.empty(), "Cannot unregister events with events pending");
    event_types.clear();
}

void Init() {
    std::unique_lock<std::mutex> lock(mutex);
    std::fill(downcount.begin(), downcount.end(), MAX_SLICE_LENGTH);
    slice_length = MAX_SLICE_LENGTH;
    next_slice_length = MAX_SLICE_LENGTH;
    global_timer = 0;

    // The time between CoreTiming being intialized and the first call to Advance() is considered
    // the slice boundary between slice -1 and slice 0. Dispatcher loops must call Advance() before
    // executing the first cycle of each slice to prepare the slice length and downcount for
    // that slice.
    is_global_timer_sane = true;

    event_fifo_id = 0;
    ev_lost = RegisterEvent("_lost_event", &EmptyTimedCallback);
}

void Shutdown() {
    MoveEvents();
    ClearPendingEvents();
    UnregisterAllEvents();
}

// This should only be called from the CPU thread. If you are calling
// it from any other thread, you are doing something evil
u64 GetTicks() {
    std::unique_lock<std::mutex> lock(mutex);
    u64 ticks = static_cast<u64>(global_timer);
    if (!is_global_timer_sane) {
        ticks += slice_length - downcount[0];
    }
    return ticks;
}

void AddTicks(u64 ticks) {
    std::unique_lock<std::mutex> lock(mutex);
    downcount[Core::System::GetInstance().CurrentCoreIndex()] -= static_cast<int>(ticks);
}

void ClearPendingEvents() {
    event_queue.clear();
}

void ScheduleEvent(s64 cycles_into_future, const EventType* event_type, u64 userdata) {
    ASSERT(event_type != nullptr);
    s64 timeout = GetTicks() + cycles_into_future;
    // If this event needs to be scheduled before the next advance(), force one early
    if (!is_global_timer_sane)
        ForceExceptionCheck(cycles_into_future);
    event_queue.emplace_back(Event{timeout, event_fifo_id++, userdata, event_type});
    std::push_heap(event_queue.begin(), event_queue.end(), std::greater<>());
}

void ScheduleEventThreadsafe(s64 cycles_into_future, const EventType* event_type, u64 userdata) {
    std::unique_lock<std::mutex> lock(mutex);
    ts_queue.Push(Event{global_timer + cycles_into_future, 0, userdata, event_type});
}

void UnscheduleEvent(const EventType* event_type, u64 userdata) {
    auto itr = std::remove_if(event_queue.begin(), event_queue.end(), [&](const Event& e) {
        return e.type == event_type && e.userdata == userdata;
    });

    // Removing random items breaks the invariant so we have to re-establish it.
    if (itr != event_queue.end()) {
        event_queue.erase(itr, event_queue.end());
        std::make_heap(event_queue.begin(), event_queue.end(), std::greater<>());
    }
}

void RemoveEvent(const EventType* event_type) {
    auto itr = std::remove_if(event_queue.begin(), event_queue.end(),
                              [&](const Event& e) { return e.type == event_type; });

    // Removing random items breaks the invariant so we have to re-establish it.
    if (itr != event_queue.end()) {
        event_queue.erase(itr, event_queue.end());
        std::make_heap(event_queue.begin(), event_queue.end(), std::greater<>());
    }
}

void RemoveNormalAndThreadsafeEvent(const EventType* event_type) {
    MoveEvents();
    RemoveEvent(event_type);
}

void ForceExceptionCheck(s64 cycles) {
    cycles = std::max<s64>(0, cycles);
    std::unique_lock<std::mutex> lock(mutex);
    if (downcount[0] > cycles) {
        // downcount is always (much) smaller than MAX_INT so we can safely cast cycles to an int
        // here. Account for cycles already executed by adjusting the g.slice_length
        s64 crop_time = downcount[0] - static_cast<int>(cycles);
        slice_length -= crop_time;
        downcount[0] = static_cast<int>(cycles);
        if (next_slice_length <= 0)
            next_slice_length = MAX_SLICE_LENGTH;
    }
}

void MoveEvents() {
    for (Event ev; ts_queue.Pop(ev);) {
        ev.fifo_order = event_fifo_id++;
        event_queue.emplace_back(std::move(ev));
        std::push_heap(event_queue.begin(), event_queue.end(), std::greater<>());
    }
}

void Advance() {
    std::unique_lock<std::mutex> lock(mutex);
    size_t current_core = Core::System::GetInstance().CurrentCoreIndex();
    if (current_core != 0) {
        downcount[current_core] = MAX_SLICE_LENGTH;
        return;
    }

    MoveEvents();

    int cycles_executed = slice_length - downcount[0];
    global_timer += cycles_executed;
    slice_length = next_slice_length;
    next_slice_length = MAX_SLICE_LENGTH;

    is_global_timer_sane = true;

    while (!event_queue.empty() && event_queue.front().time <= global_timer) {
        Event evt = std::move(event_queue.front());
        std::pop_heap(event_queue.begin(), event_queue.end(), std::greater<>());
        event_queue.pop_back();
        evt.type->callback(evt.userdata, static_cast<int>(global_timer - evt.time));
    }

    is_global_timer_sane = false;

    // Still events left (scheduled in the future)
    if (!event_queue.empty()) {
        slice_length = static_cast<int>(
            std::min<s64>(event_queue.front().time - global_timer, MAX_SLICE_LENGTH));
    }

    downcount[0] = slice_length;
    next_slice_length = MAX_SLICE_LENGTH - slice_length;
}

void Idle() {
    std::unique_lock<std::mutex> lock(mutex);
    downcount[Core::System::GetInstance().CurrentCoreIndex()] = 0;
}

bool MainSliceWasCropped() {
    std::unique_lock<std::mutex> lock(mutex);
    return next_slice_length != MAX_SLICE_LENGTH;
}

std::chrono::microseconds GetGlobalTimeUs() {
    return std::chrono::microseconds{GetTicks() * 1000000 / BASE_CLOCK_RATE};
}

int GetDowncount() {
    std::unique_lock<std::mutex> lock(mutex);
    return downcount[Core::System::GetInstance().CurrentCoreIndex()];
}

} // namespace CoreTiming
