// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <map>
#include <vector>
#include <boost/range/algorithm_ext/erase.hpp>
#include "common/assert.h"
#include "core/core.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/mutex.h"
#include "core/hle/kernel/object_address_table.h"
#include "core/hle/kernel/thread.h"

namespace Kernel {

void ReleaseThreadMutexes(Thread* thread) {
    for (auto& mtx : thread->held_mutexes) {
        mtx->lock_count = 0;
        mtx->holding_thread = nullptr;
        mtx->WakeupAllWaitingThreads();
    }
    thread->held_mutexes.clear();
}

Mutex::Mutex() {}
Mutex::~Mutex() {}

SharedPtr<Mutex> Mutex::Create(SharedPtr<Kernel::Thread> holding_thread, VAddr guest_addr,
                               std::string name) {
    SharedPtr<Mutex> mutex(new Mutex);

    mutex->lock_count = 0;
    mutex->guest_addr = guest_addr;
    mutex->name = std::move(name);
    mutex->holding_thread = nullptr;

    // If mutex was initialized with a holding thread, acquire it by the holding thread
    if (holding_thread) {
        mutex->Acquire(holding_thread.get());
    }

    // Mutexes are referenced by guest address, so track this in the kernel
    g_object_address_table.Insert(guest_addr, mutex);

    // Verify that the created mutex matches the guest state for the mutex
    mutex->VerifyGuestState();

    return mutex;
}

bool Mutex::ShouldWait(Thread* thread) const {
    return lock_count > 0 && thread != holding_thread;
}

void Mutex::Acquire(Thread* thread) {
    ASSERT_MSG(!ShouldWait(thread), "object unavailable!");

    // Actually "acquire" the mutex only if we don't already have it
    if (lock_count == 0) {
        priority = thread->current_priority;
        thread->held_mutexes.insert(this);
        holding_thread = thread;
        thread->UpdatePriority();
        UpdateGuestState();
        Core::System::GetInstance().PrepareReschedule();
    }

    lock_count++;
}

ResultCode Mutex::Release(Thread* thread) {
    // We can only release the mutex if it's held by the calling thread.
    if (thread != holding_thread) {
        if (holding_thread) {
            LOG_ERROR(
                Kernel,
                "Tried to release a mutex (owned by thread id %u) from a different thread id %u",
                holding_thread->thread_id, thread->thread_id);
        }
        // TODO(bunnei): Use correct error code
        return ResultCode(-1);
    }

    // Note: It should not be possible for the situation where the mutex has a holding thread with a
    // zero lock count to occur. The real kernel still checks for this, so we do too.
    if (lock_count <= 0) {
        // TODO(bunnei): Use correct error code
        return ResultCode(-1);
    }

    lock_count--;

    // Yield to the next thread only if we've fully released the mutex
    if (lock_count == 0) {
        holding_thread->held_mutexes.erase(this);
        holding_thread->UpdatePriority();
        holding_thread = nullptr;
        WakeupAllWaitingThreads();
        UpdateGuestState();
        Core::System::GetInstance().PrepareReschedule();
    }

    return RESULT_SUCCESS;
}

void Mutex::AddWaitingThread(SharedPtr<Thread> thread) {
    WaitObject::AddWaitingThread(thread);
    thread->pending_mutexes.insert(this);
    UpdatePriority();
    UpdateGuestState();
}

void Mutex::RemoveWaitingThread(Thread* thread) {
    WaitObject::RemoveWaitingThread(thread);
    thread->pending_mutexes.erase(this);
    UpdatePriority();
    UpdateGuestState();
}

void Mutex::UpdatePriority() {
    if (!holding_thread)
        return;

    u32 best_priority = THREADPRIO_LOWEST;
    for (auto& waiter : GetWaitingThreads()) {
        if (waiter->current_priority < best_priority)
            best_priority = waiter->current_priority;
    }

    if (best_priority != priority) {
        priority = best_priority;
        holding_thread->UpdatePriority();
    }
}

void Mutex::UpdateGuestState() {
    GuestState guest_state{Memory::Read32(guest_addr)};
    guest_state.has_waiters.Assign(!GetWaitingThreads().empty());
    guest_state.holding_thread_handle.Assign(holding_thread ? holding_thread->guest_handle : 0);
    Memory::Write32(guest_addr, guest_state.raw);
}

void Mutex::VerifyGuestState() {
    GuestState guest_state{Memory::Read32(guest_addr)};
    ASSERT(guest_state.has_waiters == !GetWaitingThreads().empty());
    ASSERT(guest_state.holding_thread_handle == holding_thread->guest_handle);
}

} // namespace Kernel
