// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "common/assert.h"
#include "common/fiber.h"
#include "video_core/control/scheduler.h"
#include "video_core/dma_pusher.h"
#include "video_core/gpu.h"

namespace Tegra::Control {

struct GPFifoContext {
    bool is_active;
    bool is_running;
    std::shared_ptr<Common::Fiber> context;
    std::deque<CommandList> pending_work;
    std::mutex guard;
    s32 bind_id;
    std::shared_ptr<ChannelState> info;
    size_t yield_count;
    size_t scheduled_count;
};

struct Scheduler::SchedulerImpl {
    // Fifos
    std::map<u32, std::list<size_t>, std::greater<u32>> schedule_priority_queue;
    std::unordered_map<s32, size_t> channel_gpfifo_ids;
    std::deque<GPFifoContext> gpfifos;
    std::deque<size_t> free_fifos;

    // Scheduling
    std::mutex scheduling_guard;
    std::shared_ptr<Common::Fiber> master_control;
    bool must_reschedule{};
    GPFifoContext* current_fifo{};
};

Scheduler::Scheduler(GPU& gpu_) : gpu{gpu_} {
    impl = std::make_unique<SchedulerImpl>();
}

Scheduler::~Scheduler() = default;

void Scheduler::Init() {
    impl->master_control = Common::Fiber::ThreadToFiber();
}

void Scheduler::Resume() {
    while (UpdateHighestPriorityChannel()) {
        impl->current_fifo->scheduled_count++;
        Common::Fiber::YieldTo(impl->master_control, *impl->current_fifo->context);
    }
}

bool Scheduler::UpdateHighestPriorityChannel() {
    std::scoped_lock lk(impl->scheduling_guard);

    // Clear needs to schedule state.
    impl->must_reschedule = false;

    // By default, we don't have a channel to schedule.
    impl->current_fifo = nullptr;

    // Check each level to see if we can schedule.
    for (auto& level : impl->schedule_priority_queue) {
        if (ScheduleLevel(level.second)) {
            return true;
        }
    }

    // Nothing to schedule.
    return false;
}

bool Scheduler::ScheduleLevel(std::list<size_t>& queue) {
    bool found_anything = false;
    size_t min_schedule_count = std::numeric_limits<size_t>::max();
    for (auto id : queue) {
        auto& fifo = impl->gpfifos[id];
        std::scoped_lock lk(fifo.guard);

        // With no pending work and nothing running, this channel can't be scheduled.
        if (fifo.pending_work.empty() && !fifo.is_running) {
            continue;
        }
        // Prioritize channels at current priority which have been run the least.
        if (fifo.scheduled_count > min_schedule_count) {
            continue;
        }

        // Try not to select the same channel we just yielded from.
        if (fifo.scheduled_count < fifo.yield_count) {
            fifo.scheduled_count++;
            continue;
        }

        // Update best selection.
        min_schedule_count = fifo.scheduled_count;
        impl->current_fifo = &fifo;
        found_anything = true;
    }
    return found_anything;
}

void Scheduler::ChangePriority(s32 channel_id, u32 new_priority) {
    std::scoped_lock lk(impl->scheduling_guard);
    // Ensure we are tracking this channel.
    auto fifo_it = impl->channel_gpfifo_ids.find(channel_id);
    if (fifo_it == impl->channel_gpfifo_ids.end()) {
        return;
    }

    // Get the fifo and update its priority.
    const size_t fifo_id = fifo_it->second;
    auto& fifo = impl->gpfifos[fifo_id];
    const auto old_priority = std::exchange(fifo.info->priority, new_priority);

    // Create the new level if needed.
    impl->schedule_priority_queue.try_emplace(new_priority);

    // Remove the old level and add to the new level.
    impl->schedule_priority_queue[new_priority].push_back(fifo_id);
    impl->schedule_priority_queue[old_priority].remove_if(
        [fifo_id](size_t id) { return id == fifo_id; });
}

void Scheduler::Yield() {
    ASSERT(impl->current_fifo != nullptr);

    // Set yield count higher
    impl->current_fifo->yield_count = impl->current_fifo->scheduled_count + 1;
    Common::Fiber::YieldTo(impl->current_fifo->context, *impl->master_control);
    gpu.BindChannel(impl->current_fifo->bind_id);
}

void Scheduler::CheckStatus() {
    {
        std::unique_lock lk(impl->scheduling_guard);
        // If no reschedule is needed, don't transfer control
        if (!impl->must_reschedule) {
            return;
        }
    }
    // Transfer control to the scheduler
    Common::Fiber::YieldTo(impl->current_fifo->context, *impl->master_control);
    gpu.BindChannel(impl->current_fifo->bind_id);
}

void Scheduler::Push(s32 channel, CommandList&& entries) {
    std::scoped_lock lk(impl->scheduling_guard);
    // Get and ensure we have this channel.
    auto it = impl->channel_gpfifo_ids.find(channel);
    ASSERT(it != impl->channel_gpfifo_ids.end());
    auto gpfifo_id = it->second;
    auto& fifo = impl->gpfifos[gpfifo_id];
    // Add the new new work to the channel.
    {
        std::scoped_lock lk2(fifo.guard);
        fifo.pending_work.emplace_back(std::move(entries));
    }

    // If the current running FIFO is null or the one being pushed to then
    // just return
    if (impl->current_fifo == nullptr || impl->current_fifo == &fifo) {
        return;
    }

    // If the current fifo has higher or equal priority to the current fifo then return
    if (impl->current_fifo->info->priority >= fifo.info->priority) {
        return;
    }
    // Mark scheduler update as required.
    impl->must_reschedule = true;
}

void Scheduler::ChannelLoop(size_t gpfifo_id, s32 channel_id) {
    auto& fifo = impl->gpfifos[gpfifo_id];
    auto* channel_state = fifo.info.get();
    const auto SendToPuller = [&] {
        std::scoped_lock lk(fifo.guard);
        if (fifo.pending_work.empty()) {
            // Stop if no work available.
            fifo.is_running = false;
            return false;
        }
        // Otherwise, send work to puller and mark as running.
        CommandList&& entries = std::move(fifo.pending_work.front());
        channel_state->dma_pusher->Push(std::move(entries));
        fifo.pending_work.pop_front();
        fifo.is_running = true;
        // Succeed.
        return true;
    };
    // Inform the GPU about the current channel.
    gpu.BindChannel(channel_id);
    while (true) {
        while (SendToPuller()) {
            // Execute.
            channel_state->dma_pusher->DispatchCalls();
            // Reschedule.
            CheckStatus();
        }
        // Return to host execution when all work is completed.
        Common::Fiber::YieldTo(fifo.context, *impl->master_control);
        // Inform the GPU about the current channel.
        gpu.BindChannel(channel_id);
    }
}

void Scheduler::DeclareChannel(std::shared_ptr<ChannelState> new_channel) {
    s32 channel = new_channel->bind_id;
    std::unique_lock lk(impl->scheduling_guard);

    size_t new_fifo_id;
    if (!impl->free_fifos.empty()) {
        new_fifo_id = impl->free_fifos.front();
        impl->free_fifos.pop_front();
    } else {
        new_fifo_id = impl->gpfifos.size();
        impl->gpfifos.emplace_back();
    }
    auto& new_fifo = impl->gpfifos[new_fifo_id];
    impl->channel_gpfifo_ids[channel] = new_fifo_id;
    new_fifo.is_active = true;
    new_fifo.bind_id = channel;
    new_fifo.pending_work.clear();
    new_fifo.info = new_channel;
    new_fifo.scheduled_count = 0;
    new_fifo.yield_count = 0;
    new_fifo.is_running = false;
    impl->schedule_priority_queue.try_emplace(new_channel->priority);
    impl->schedule_priority_queue[new_channel->priority].push_back(new_fifo_id);
    std::function<void()> callback = std::bind(&Scheduler::ChannelLoop, this, new_fifo_id, channel);
    new_fifo.context = std::make_shared<Common::Fiber>(std::move(callback));
}

} // namespace Tegra::Control
