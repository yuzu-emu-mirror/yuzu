// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <map>
#include <vector>
#include <boost/range/algorithm_ext/erase.hpp>
#include "common/assert.h"
#include "core/core.h"
#include "core/hle/kernel/errors.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/mutex.h"
#include "core/hle/kernel/object_address_table.h"
#include "core/hle/kernel/thread.h"

namespace Kernel {

/// Returns the number of threads that are waiting for a mutex, and the highest priority one among
/// those.
static std::pair<SharedPtr<Thread>, u32> GetHighestPriorityMutexWaitingThread(VAddr mutex_addr) {
    auto& thread_list = Core::System::GetInstance().Scheduler().GetThreadList();

    SharedPtr<Thread> highest_priority_thread;
    u32 num_waiters = 0;

    for (auto& thread : thread_list) {
        if (thread->mutex_wait_address != mutex_addr)
            continue;

        ASSERT(thread->status == THREADSTATUS_WAIT_MUTEX);

        ++num_waiters;
        if (highest_priority_thread == nullptr ||
            thread->GetPriority() < highest_priority_thread->GetPriority()) {
            highest_priority_thread = thread;
        }
    }

    return {highest_priority_thread, num_waiters};
}

void ReleaseThreadMutexes(Thread* thread) {
    for (auto& mtx : thread->held_mutexes) {
        mtx->SetHasWaiters(false);
        mtx->SetHoldingThread(nullptr);
        mtx->WakeupAllWaitingThreads();
    }
    thread->held_mutexes.clear();
}

Mutex::Mutex() {}
Mutex::~Mutex() {}

SharedPtr<Mutex> Mutex::Create(SharedPtr<Kernel::Thread> holding_thread, VAddr guest_addr,
                               std::string name) {
    SharedPtr<Mutex> mutex(new Mutex);

    mutex->guest_addr = guest_addr;
    mutex->name = std::move(name);

    // If mutex was initialized with a holding thread, acquire it by the holding thread
    if (holding_thread) {
        mutex->Acquire(holding_thread.get());
    }

    // Mutexes are referenced by guest address, so track this in the kernel
    g_object_address_table.Insert(guest_addr, mutex);

    return mutex;
}

bool Mutex::ShouldWait(Thread* thread) const {
    auto holding_thread = GetHoldingThread();
    return holding_thread != nullptr && thread != holding_thread;
}

void Mutex::Acquire(Thread* thread) {
    ASSERT_MSG(!ShouldWait(thread), "object unavailable!");

    priority = thread->current_priority;
    thread->held_mutexes.insert(this);
    SetHoldingThread(thread);
    thread->UpdatePriority();
    Core::System::GetInstance().PrepareReschedule();
}

ResultCode Mutex::Release(Thread* thread) {
    auto holding_thread = GetHoldingThread();
    ASSERT(holding_thread);

    // We can only release the mutex if it's held by the calling thread.
    ASSERT(thread == holding_thread);

    holding_thread->held_mutexes.erase(this);
    holding_thread->UpdatePriority();
    SetHoldingThread(nullptr);
    SetHasWaiters(!GetWaitingThreads().empty());
    WakeupAllWaitingThreads();
    Core::System::GetInstance().PrepareReschedule();

    return RESULT_SUCCESS;
}

void Mutex::AddWaitingThread(SharedPtr<Thread> thread) {
    WaitObject::AddWaitingThread(thread);
    thread->pending_mutexes.insert(this);
    SetHasWaiters(true);
    UpdatePriority();
}

void Mutex::RemoveWaitingThread(Thread* thread) {
    WaitObject::RemoveWaitingThread(thread);
    thread->pending_mutexes.erase(this);
    if (!GetHasWaiters())
        SetHasWaiters(!GetWaitingThreads().empty());
    UpdatePriority();
}

void Mutex::UpdatePriority() {
    if (!GetHoldingThread())
        return;

    u32 best_priority = THREADPRIO_LOWEST;
    for (auto& waiter : GetWaitingThreads()) {
        if (waiter->current_priority < best_priority)
            best_priority = waiter->current_priority;
    }

    if (best_priority != priority) {
        priority = best_priority;
        GetHoldingThread()->UpdatePriority();
    }
}

Handle Mutex::GetOwnerHandle() const {
    GuestState guest_state{Memory::Read32(guest_addr)};
    return guest_state.holding_thread_handle;
}

SharedPtr<Thread> Mutex::GetHoldingThread() const {
    GuestState guest_state{Memory::Read32(guest_addr)};
    return g_handle_table.Get<Thread>(guest_state.holding_thread_handle);
}

void Mutex::SetHoldingThread(SharedPtr<Thread> thread) {
    GuestState guest_state{Memory::Read32(guest_addr)};
    guest_state.holding_thread_handle.Assign(thread ? thread->guest_handle : 0);
    Memory::Write32(guest_addr, guest_state.raw);
}

bool Mutex::GetHasWaiters() const {
    GuestState guest_state{Memory::Read32(guest_addr)};
    return guest_state.has_waiters != 0;
}

void Mutex::SetHasWaiters(bool has_waiters) {
    GuestState guest_state{Memory::Read32(guest_addr)};
    guest_state.has_waiters.Assign(has_waiters ? 1 : 0);
    Memory::Write32(guest_addr, guest_state.raw);
}

ResultCode Mutex::TryAcquire(VAddr address, Handle holding_thread_handle,
                             Handle requesting_thread_handle) {
    // The mutex address must be 4-byte aligned
    if ((address % sizeof(u32)) != 0) {
        return ResultCode(ErrorModule::Kernel, ErrCodes::MisalignedAddress);
    }

    SharedPtr<Thread> holding_thread = g_handle_table.Get<Thread>(holding_thread_handle);
    SharedPtr<Thread> requesting_thread = g_handle_table.Get<Thread>(requesting_thread_handle);

    // TODO(Subv): It is currently unknown if it is possible to lock a mutex in behalf of another
    // thread.
    ASSERT(requesting_thread == GetCurrentThread());

    u32 addr_value = Memory::Read32(address);

    // If the mutex isn't being held, just return success.
    if (addr_value != (holding_thread_handle | Mutex::MutexHasWaitersFlag)) {
        return RESULT_SUCCESS;
    }

    if (holding_thread == nullptr)
        return ERR_INVALID_HANDLE;

    // Wait until the mutex is released
    requesting_thread->mutex_wait_address = address;
    requesting_thread->wait_handle = requesting_thread_handle;

    requesting_thread->status = THREADSTATUS_WAIT_MUTEX;
    requesting_thread->wakeup_callback = nullptr;

    Core::System::GetInstance().PrepareReschedule();

    return RESULT_SUCCESS;
}

ResultCode Mutex::Release(VAddr address) {
    // The mutex address must be 4-byte aligned
    if ((address % sizeof(u32)) != 0) {
        return ResultCode(ErrorModule::Kernel, ErrCodes::MisalignedAddress);
    }

    auto [thread, num_waiters] = GetHighestPriorityMutexWaitingThread(address);

    // There are no more threads waiting for the mutex, release it completely.
    if (thread == nullptr) {
        Memory::Write32(address, 0);
        return RESULT_SUCCESS;
    }

    u32 mutex_value = thread->wait_handle;

    if (num_waiters >= 2) {
        // Notify the guest that there are still some threads waiting for the mutex
        mutex_value |= Mutex::MutexHasWaitersFlag;
    }

    // Grant the mutex to the next waiting thread and resume it.
    Memory::Write32(address, mutex_value);

    ASSERT(thread->status == THREADSTATUS_WAIT_MUTEX);
    thread->ResumeFromWait();

    thread->condvar_wait_address = 0;
    thread->mutex_wait_address = 0;
    thread->wait_handle = 0;

    return RESULT_SUCCESS;
}
} // namespace Kernel
