// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <queue>
#include <string>
#include "common/common_types.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/wait_object.h"
#include "core/hle/result.h"

namespace Kernel {

// TODO(Subv): This is actually a Condition Variable.
class Semaphore final : public WaitObject {
public:
    /**
     * Creates a semaphore.
     * @param guest_addr Address of the object tracking the semaphore in guest memory. If specified,
     * this semaphore will update the guest object when its state changes.
     * @param mutex_addr Optional address of a guest mutex associated with this semaphore, used by
     * the OS for implementing events.
     * @param name Optional name of semaphore.
     * @return The created semaphore.
     */
    static ResultVal<SharedPtr<Semaphore>> Create(VAddr guest_addr, VAddr mutex_addr = 0,
                                                  std::string name = "Unknown");

    std::string GetTypeName() const override {
        return "Semaphore";
    }
    std::string GetName() const override {
        return name;
    }

    static const HandleType HANDLE_TYPE = HandleType::Semaphore;
    HandleType GetHandleType() const override {
        return HANDLE_TYPE;
    }

    s32 GetAvailableCount() const;
    void SetAvailableCount(s32 value) const;

    std::string name;    ///< Name of semaphore (optional)
    VAddr guest_addr;    ///< Address of the guest semaphore value
    VAddr mutex_addr; ///< (optional) Address of guest mutex value associated with this semaphore,
                      ///< used for implementing events

    bool ShouldWait(Thread* thread) const override;
    void Acquire(Thread* thread) override;

    /**
     * Releases a slot from a semaphore.
     * @param target The number of threads to wakeup, -1 is all.
     * @return ResultCode indicating if the operation succeeded.
     */
    ResultCode Release(s32 target);

private:
    Semaphore();
    ~Semaphore() override;
};

} // namespace Kernel
