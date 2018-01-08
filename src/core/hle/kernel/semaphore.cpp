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

    semaphore->name = std::move(name);
    semaphore->guest_addr = guest_addr;
    semaphore->mutex_addr = mutex_addr;

    // Semaphores are referenced by guest address, so track this in the kernel
    g_object_address_table.Insert(guest_addr, semaphore);

    return MakeResult<SharedPtr<Semaphore>>(std::move(semaphore));
}

bool Semaphore::ShouldWait(Thread* thread) const {
    return GetAvailableCount() <= 0;
}

void Semaphore::Acquire(Thread* thread) {
    if (GetAvailableCount() <= 0)
        return;

    SetAvailableCount(GetAvailableCount() - 1);
}

ResultCode Semaphore::Release(s32 target) {
    if (target == -1) {
        // When -1, wake up all waiting threads
        SetAvailableCount(GetWaitingThreads().size());
        WakeupAllWaitingThreads();
    } else {
        // Otherwise, wake up just a single thread
        SetAvailableCount(target);
        WakeupWaitingThread(GetHighestPriorityReadyThread());
    }

    return RESULT_SUCCESS;
}

s32 Semaphore::GetAvailableCount() const {
    return Memory::Read32(guest_addr);
}

void Semaphore::SetAvailableCount(s32 value) const {
    Memory::Write32(guest_addr, value);
}

} // namespace Kernel
