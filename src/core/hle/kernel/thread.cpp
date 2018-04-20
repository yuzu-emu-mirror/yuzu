// Copyright 2014 Citra Emulator Project / PPSSPP Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cinttypes>
#include <list>
#include <vector>
#include "common/assert.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/math_util.h"
#include "common/thread_queue_list.h"
#include "core/arm/arm_interface.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/kernel/errors.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/memory.h"
#include "core/hle/kernel/mutex.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/result.h"
#include "core/memory.h"

namespace Kernel {

/// Event type for the thread wake up event
static CoreTiming::EventType* ThreadWakeupEventType = nullptr;

bool Thread::ShouldWait(Thread* thread) const {
    return status != THREADSTATUS_DEAD;
}

void Thread::Acquire(Thread* thread) {
    ASSERT_MSG(!ShouldWait(thread), "object unavailable!");
}

// TODO(yuriks): This can be removed if Thread objects are explicitly pooled in the future, allowing
//               us to simply use a pool index or similar.
static Kernel::HandleTable wakeup_callback_handle_table;

// The first available thread id at startup
static u32 next_thread_id;

/**
 * Creates a new thread ID
 * @return The new thread ID
 */
inline static u32 const NewThreadId() {
    return next_thread_id++;
}

Thread::Thread() {}
Thread::~Thread() {}

void Thread::Stop() {
    // Cancel any outstanding wakeup events for this thread
    CoreTiming::UnscheduleEvent(ThreadWakeupEventType, callback_handle);
    wakeup_callback_handle_table.Close(callback_handle);
    callback_handle = 0;

    // Clean up thread from ready queue
    // This is only needed when the thread is termintated forcefully (SVC TerminateProcess)
    if (status == THREADSTATUS_READY) {
        Core::System::GetInstance().Scheduler().UnscheduleThread(this, current_priority);
    }

    status = THREADSTATUS_DEAD;

    WakeupAllWaitingThreads();

    // Clean up any dangling references in objects that this thread was waiting for
    for (auto& wait_object : wait_objects) {
        wait_object->RemoveWaitingThread(this);
    }
    wait_objects.clear();

    // Mark the TLS slot in the thread's page as free.
    u64 tls_page = (tls_address - Memory::TLS_AREA_VADDR) / Memory::PAGE_SIZE;
    u64 tls_slot =
        ((tls_address - Memory::TLS_AREA_VADDR) % Memory::PAGE_SIZE) / Memory::TLS_ENTRY_SIZE;
    Core::CurrentProcess()->tls_slots[tls_page].reset(tls_slot);
}

void WaitCurrentThread_Sleep() {
    Thread* thread = GetCurrentThread();
    thread->status = THREADSTATUS_WAIT_SLEEP;
}

void ExitCurrentThread() {
    Thread* thread = GetCurrentThread();
    thread->Stop();
    Core::System::GetInstance().Scheduler().RemoveThread(thread);
}

/**
 * Callback that will wake up the thread it was scheduled for
 * @param thread_handle The handle of the thread that's been awoken
 * @param cycles_late The number of CPU cycles that have passed since the desired wakeup time
 */
static void ThreadWakeupCallback(u64 thread_handle, int cycles_late) {
    SharedPtr<Thread> thread = wakeup_callback_handle_table.Get<Thread>((Handle)thread_handle);
    if (thread == nullptr) {
        LOG_CRITICAL(Kernel, "Callback fired for invalid thread %08X", (Handle)thread_handle);
        return;
    }

    bool resume = true;

    if (thread->status == THREADSTATUS_WAIT_SYNCH_ANY ||
        thread->status == THREADSTATUS_WAIT_SYNCH_ALL ||
        thread->status == THREADSTATUS_WAIT_HLE_EVENT) {

        // Remove the thread from each of its waiting objects' waitlists
        for (auto& object : thread->wait_objects)
            object->RemoveWaitingThread(thread.get());
        thread->wait_objects.clear();

        // Invoke the wakeup callback before clearing the wait objects
        if (thread->wakeup_callback)
            resume = thread->wakeup_callback(ThreadWakeupReason::Timeout, thread, nullptr, 0);
    }

    if (thread->mutex_wait_address != 0 || thread->condvar_wait_address != 0 ||
        thread->wait_handle) {
        ASSERT(thread->status == THREADSTATUS_WAIT_MUTEX);
        thread->mutex_wait_address = 0;
        thread->condvar_wait_address = 0;
        thread->wait_handle = 0;
    }

    if (resume)
        thread->ResumeFromWait();
}

void Thread::WakeAfterDelay(s64 nanoseconds) {
    // Don't schedule a wakeup if the thread wants to wait forever
    if (nanoseconds == -1)
        return;

    CoreTiming::ScheduleEvent(nsToCycles(nanoseconds), ThreadWakeupEventType, callback_handle);
}

void Thread::CancelWakeupTimer() {
    CoreTiming::UnscheduleEvent(ThreadWakeupEventType, callback_handle);
}

void Thread::ResumeFromWait() {
    ASSERT_MSG(wait_objects.empty(), "Thread is waking up while waiting for objects");

    switch (status) {
    case THREADSTATUS_WAIT_SYNCH_ALL:
    case THREADSTATUS_WAIT_SYNCH_ANY:
    case THREADSTATUS_WAIT_HLE_EVENT:
    case THREADSTATUS_WAIT_SLEEP:
    case THREADSTATUS_WAIT_IPC:
    case THREADSTATUS_WAIT_MUTEX:
        break;

    case THREADSTATUS_READY:
        // The thread's wakeup callback must have already been cleared when the thread was first
        // awoken.
        ASSERT(wakeup_callback == nullptr);
        // If the thread is waiting on multiple wait objects, it might be awoken more than once
        // before actually resuming. We can ignore subsequent wakeups if the thread status has
        // already been set to THREADSTATUS_READY.
        return;

    case THREADSTATUS_RUNNING:
        DEBUG_ASSERT_MSG(false, "Thread with object id %u has already resumed.", GetObjectId());
        return;
    case THREADSTATUS_DEAD:
        // This should never happen, as threads must complete before being stopped.
        DEBUG_ASSERT_MSG(false, "Thread with object id %u cannot be resumed because it's DEAD.",
                         GetObjectId());
        return;
    }

    wakeup_callback = nullptr;

    status = THREADSTATUS_READY;
    Core::System::GetInstance().Scheduler().ScheduleThread(this, current_priority);
    Core::System::GetInstance().PrepareReschedule();
}

/**
 * Finds a free location for the TLS section of a thread.
 * @param tls_slots The TLS page array of the thread's owner process.
 * Returns a tuple of (page, slot, alloc_needed) where:
 * page: The index of the first allocated TLS page that has free slots.
 * slot: The index of the first free slot in the indicated page.
 * alloc_needed: Whether there's a need to allocate a new TLS page (All pages are full).
 */
std::tuple<u32, u32, bool> GetFreeThreadLocalSlot(std::vector<std::bitset<8>>& tls_slots) {
    // Iterate over all the allocated pages, and try to find one where not all slots are used.
    for (unsigned page = 0; page < tls_slots.size(); ++page) {
        const auto& page_tls_slots = tls_slots[page];
        if (!page_tls_slots.all()) {
            // We found a page with at least one free slot, find which slot it is
            for (unsigned slot = 0; slot < page_tls_slots.size(); ++slot) {
                if (!page_tls_slots.test(slot)) {
                    return std::make_tuple(page, slot, false);
                }
            }
        }
    }

    return std::make_tuple(0, 0, true);
}

/**
 * Resets a thread context, making it ready to be scheduled and run by the CPU
 * @param context Thread context to reset
 * @param stack_top Address of the top of the stack
 * @param entry_point Address of entry point for execution
 * @param arg User argument for thread
 */
static void ResetThreadContext(ARM_Interface::ThreadContext& context, VAddr stack_top,
                               VAddr entry_point, u64 arg) {
    memset(&context, 0, sizeof(ARM_Interface::ThreadContext));

    context.cpu_registers[0] = arg;
    context.pc = entry_point;
    context.sp = stack_top;
    context.cpsr = 0;
    context.fpscr = 0;
}

ResultVal<SharedPtr<Thread>> Thread::Create(std::string name, VAddr entry_point, u32 priority,
                                            u64 arg, s32 processor_id, VAddr stack_top,
                                            SharedPtr<Process> owner_process) {
    // Check if priority is in ranged. Lowest priority -> highest priority id.
    if (priority > THREADPRIO_LOWEST) {
        LOG_ERROR(Kernel_SVC, "Invalid thread priority: %u", priority);
        return ERR_OUT_OF_RANGE;
    }

    if (processor_id > THREADPROCESSORID_MAX) {
        LOG_ERROR(Kernel_SVC, "Invalid processor id: %d", processor_id);
        return ERR_OUT_OF_RANGE_KERNEL;
    }

    // TODO(yuriks): Other checks, returning 0xD9001BEA

    if (!Memory::IsValidVirtualAddress(*owner_process, entry_point)) {
        LOG_ERROR(Kernel_SVC, "(name=%s): invalid entry %016" PRIx64, name.c_str(), entry_point);
        // TODO (bunnei): Find the correct error code to use here
        return ResultCode(-1);
    }

    SharedPtr<Thread> thread(new Thread);

    Core::System::GetInstance().Scheduler().AddThread(thread, priority);

    thread->thread_id = NewThreadId();
    thread->status = THREADSTATUS_DORMANT;
    thread->entry_point = entry_point;
    thread->stack_top = stack_top;
    thread->nominal_priority = thread->current_priority = priority;
    thread->last_running_ticks = CoreTiming::GetTicks();
    thread->processor_id = processor_id;
    thread->wait_objects.clear();
    thread->mutex_wait_address = 0;
    thread->condvar_wait_address = 0;
    thread->wait_handle = 0;
    thread->name = std::move(name);
    thread->callback_handle = wakeup_callback_handle_table.Create(thread).Unwrap();
    thread->owner_process = owner_process;

    // Find the next available TLS index, and mark it as used
    auto& tls_slots = owner_process->tls_slots;
    bool needs_allocation = true;
    u32 available_page; // Which allocated page has free space
    u32 available_slot; // Which slot within the page is free

    std::tie(available_page, available_slot, needs_allocation) = GetFreeThreadLocalSlot(tls_slots);

    if (needs_allocation) {
        // There are no already-allocated pages with free slots, lets allocate a new one.
        // TLS pages are allocated from the BASE region in the linear heap.
        MemoryRegionInfo* memory_region = GetMemoryRegion(MemoryRegion::BASE);
        auto& linheap_memory = memory_region->linear_heap_memory;

        if (linheap_memory->size() + Memory::PAGE_SIZE > memory_region->size) {
            LOG_ERROR(Kernel_SVC,
                      "Not enough space in region to allocate a new TLS page for thread");
            return ERR_OUT_OF_MEMORY;
        }

        size_t offset = linheap_memory->size();

        // Allocate some memory from the end of the linear heap for this region.
        linheap_memory->insert(linheap_memory->end(), Memory::PAGE_SIZE, 0);
        memory_region->used += Memory::PAGE_SIZE;
        owner_process->linear_heap_used += Memory::PAGE_SIZE;

        tls_slots.emplace_back(0); // The page is completely available at the start
        available_page = static_cast<u32>(tls_slots.size() - 1);
        available_slot = 0; // Use the first slot in the new page

        auto& vm_manager = owner_process->vm_manager;
        vm_manager.RefreshMemoryBlockMappings(linheap_memory.get());

        // Map the page to the current process' address space.
        // TODO(Subv): Find the correct MemoryState for this region.
        vm_manager.MapMemoryBlock(Memory::TLS_AREA_VADDR + available_page * Memory::PAGE_SIZE,
                                  linheap_memory, offset, Memory::PAGE_SIZE,
                                  MemoryState::ThreadLocal);
    }

    // Mark the slot as used
    tls_slots[available_page].set(available_slot);
    thread->tls_address = Memory::TLS_AREA_VADDR + available_page * Memory::PAGE_SIZE +
                          available_slot * Memory::TLS_ENTRY_SIZE;

    // TODO(peachum): move to ScheduleThread() when scheduler is added so selected core is used
    // to initialize the context
    ResetThreadContext(thread->context, stack_top, entry_point, arg);

    return MakeResult<SharedPtr<Thread>>(std::move(thread));
}

void Thread::SetPriority(u32 priority) {
    ASSERT_MSG(priority <= THREADPRIO_LOWEST && priority >= THREADPRIO_HIGHEST,
               "Invalid priority value.");
    Core::System::GetInstance().Scheduler().SetThreadPriority(this, priority);
    nominal_priority = current_priority = priority;
}

void Thread::UpdatePriority() {
    u32 best_priority = nominal_priority;
    for (auto& mutex : held_mutexes) {
        if (mutex->priority < best_priority)
            best_priority = mutex->priority;
    }
    BoostPriority(best_priority);
}

void Thread::BoostPriority(u32 priority) {
    Core::System::GetInstance().Scheduler().SetThreadPriority(this, priority);
    current_priority = priority;
}

SharedPtr<Thread> SetupMainThread(VAddr entry_point, u32 priority,
                                  SharedPtr<Process> owner_process) {
    // Setup page table so we can write to memory
    SetCurrentPageTable(&Core::CurrentProcess()->vm_manager.page_table);

    // Initialize new "main" thread
    auto thread_res = Thread::Create("main", entry_point, priority, 0, THREADPROCESSORID_0,
                                     Memory::STACK_AREA_VADDR_END, owner_process);

    SharedPtr<Thread> thread = std::move(thread_res).Unwrap();

    // Register 1 must be a handle to the main thread
    thread->guest_handle = Kernel::g_handle_table.Create(thread).Unwrap();

    thread->context.cpu_registers[1] = thread->guest_handle;

    // Threads by default are dormant, wake up the main thread so it runs when the scheduler fires
    thread->ResumeFromWait();

    return thread;
}

void Thread::SetWaitSynchronizationResult(ResultCode result) {
    context.cpu_registers[0] = result.raw;
}

void Thread::SetWaitSynchronizationOutput(s32 output) {
    context.cpu_registers[1] = output;
}

s32 Thread::GetWaitObjectIndex(WaitObject* object) const {
    ASSERT_MSG(!wait_objects.empty(), "Thread is not waiting for anything");
    auto match = std::find(wait_objects.rbegin(), wait_objects.rend(), object);
    return static_cast<s32>(std::distance(match, wait_objects.rend()) - 1);
}

VAddr Thread::GetCommandBufferAddress() const {
    // Offset from the start of TLS at which the IPC command buffer begins.
    static constexpr int CommandHeaderOffset = 0x80;
    return GetTLSAddress() + CommandHeaderOffset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Gets the current thread
 */
Thread* GetCurrentThread() {
    return Core::System::GetInstance().Scheduler().GetCurrentThread();
}

void ThreadingInit() {
    ThreadWakeupEventType = CoreTiming::RegisterEvent("ThreadWakeupCallback", ThreadWakeupCallback);
    next_thread_id = 1;
}

void ThreadingShutdown() {
    Kernel::ClearProcessList();
}

} // namespace Kernel
