// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "core/hle/kernel/errors.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/object_address_table.h"
#include "core/hle/kernel/semaphore.h"
#include "core/hle/kernel/thread.h"

namespace Kernel {

Semaphore::Semaphore() {}
Semaphore::~Semaphore() {}

ResultVal<SharedPtr<Semaphore>> Semaphore::Create(VAddr guest_addr, VAddr mutex_addr,
                                                  std::string name) {
    SharedPtr<Semaphore> semaphore(new Semaphore);

    // When the semaphore is created, some slots are reserved for other threads,
    // and the rest is reserved for the caller thread;
    semaphore->available_count = Memory::Read32(guest_addr);
    semaphore->name = std::move(name);
    semaphore->guest_addr = guest_addr;
    semaphore->mutex_addr = mutex_addr;

    // Semaphores are referenced by guest address, so track this in the kernel
    g_object_address_table.Insert(guest_addr, semaphore);

    return MakeResult<SharedPtr<Semaphore>>(std::move(semaphore));
}

bool Semaphore::ShouldWait(Thread* thread) const {
    return available_count <= 0;
}

void Semaphore::Acquire(Thread* thread) {
    if (available_count <= 0)
        return;

    --available_count;
    UpdateGuestState();
}

ResultCode Semaphore::Release(s32 target) {
    ++available_count;
    UpdateGuestState();

    if (target == -1) {
        // When -1, wake up all waiting threads
        WakeupAllWaitingThreads();
    } else {
        // Otherwise, wake up just a single thread
        WakeupWaitingThread(GetHighestPriorityReadyThread());
    }

    return RESULT_SUCCESS;
}

void Semaphore::UpdateGuestState() {
    Memory::Write32(guest_addr, available_count);
}

} // namespace Kernel
