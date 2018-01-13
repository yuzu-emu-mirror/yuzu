// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "core/hle/kernel/condition_variable.h"
#include "core/hle/kernel/errors.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/object_address_table.h"
#include "core/hle/kernel/thread.h"

namespace Kernel {

ConditionVariable::ConditionVariable() {}
ConditionVariable::~ConditionVariable() {}

ResultVal<SharedPtr<ConditionVariable>> ConditionVariable::Create(VAddr guest_addr,
                                                                  VAddr mutex_addr,
                                                                  std::string name) {
    SharedPtr<ConditionVariable> condition_variable(new ConditionVariable);

    condition_variable->name = std::move(name);
    condition_variable->guest_addr = guest_addr;
    condition_variable->mutex_addr = mutex_addr;

    // Condition variables are referenced by guest address, so track this in the kernel
    g_object_address_table.Insert(guest_addr, condition_variable);

    return MakeResult<SharedPtr<ConditionVariable>>(std::move(condition_variable));
}

bool ConditionVariable::ShouldWait(Thread* thread) const {
    return GetAvailableCount() <= 0;
}

void ConditionVariable::Acquire(Thread* thread) {
    if (GetAvailableCount() <= 0)
        return;

    SetAvailableCount(GetAvailableCount() - 1);
}

ResultCode ConditionVariable::Release(s32 target) {
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

s32 ConditionVariable::GetAvailableCount() const {
    return Memory::Read32(guest_addr);
}

void ConditionVariable::SetAvailableCount(s32 value) const {
    Memory::Write32(guest_addr, value);
}

} // namespace Kernel
