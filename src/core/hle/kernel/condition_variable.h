// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <queue>
#include "common/common_types.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/wait_object.h"
#include "core/hle/result.h"

namespace Kernel {

class ConditionVariable final : public WaitObject {
public:
    /**
     * Creates a condition variable.
     * @param guest_addr Address of the object tracking the condition variable in guest memory. If
     * specified, this condition variable will update the guest object when its state changes.
     * @param name Optional name of condition variable.
     * @return The created condition variable.
     */
    static ResultVal<SharedPtr<ConditionVariable>> Create(VAddr guest_addr,
                                                          std::string name = "Unknown");

    std::string GetTypeName() const override {
        return "ConditionVariable";
    }
    std::string GetName() const override {
        return name;
    }

    static const HandleType HANDLE_TYPE = HandleType::ConditionVariable;
    HandleType GetHandleType() const override {
        return HANDLE_TYPE;
    }

    s32 GetAvailableCount() const;
    void SetAvailableCount(s32 value) const;

    std::string name; ///< Name of condition variable (optional)
    VAddr guest_addr; ///< Address of the guest condition variable value
    VAddr mutex_addr; ///< (optional) Address of guest mutex value associated with this condition
                      ///< variable, used for implementing events

    bool ShouldWait(Thread* thread) const override;
    void Acquire(Thread* thread) override;

    /**
     * Releases a slot from a condition variable.
     * @param target The number of threads to wakeup, -1 is all.
     * @return ResultCode indicating if the operation succeeded.
     */
    ResultCode Release(s32 target);

private:
    ConditionVariable();
    ~ConditionVariable() override;
};

} // namespace Kernel
