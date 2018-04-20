// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/wait_object.h"
#include "core/hle/result.h"

namespace Kernel {

class Thread;

class Mutex final : public WaitObject {
public:
    /**
     * Creates a mutex.
     * @param holding_thread Specifies a thread already holding the mutex. If not nullptr, this
     * thread will acquire the mutex.
     * @param guest_addr Address of the object tracking the mutex in guest memory. If specified,
     * this mutex will update the guest object when its state changes.
     * @param name Optional name of mutex
     * @return Pointer to new Mutex object
     */
    static SharedPtr<Mutex> Create(SharedPtr<Kernel::Thread> holding_thread, VAddr guest_addr = 0,
                                   std::string name = "Unknown");

    std::string GetTypeName() const override {
        return "Mutex";
    }
    std::string GetName() const override {
        return name;
    }

    static const HandleType HANDLE_TYPE = HandleType::Mutex;
    HandleType GetHandleType() const override {
        return HANDLE_TYPE;
    }

    u32 priority;     ///< The priority of the mutex, used for priority inheritance.
    std::string name; ///< Name of mutex (optional)
    VAddr guest_addr; ///< Address of the guest mutex value

    /**
     * Elevate the mutex priority to the best priority
     * among the priorities of all its waiting threads.
     */
    void UpdatePriority();

    bool ShouldWait(Thread* thread) const override;
    void Acquire(Thread* thread) override;

    void AddWaitingThread(SharedPtr<Thread> thread) override;
    void RemoveWaitingThread(Thread* thread) override;

    /**
     * Attempts to release the mutex from the specified thread.
     * @param thread Thread that wants to release the mutex.
     * @returns The result code of the operation.
     */
    ResultCode Release(Thread* thread);

    /// Gets the handle to the holding process stored in the guest state.
    Handle GetOwnerHandle() const;

    /// Gets the Thread pointed to by the owner handle
    SharedPtr<Thread> GetHoldingThread() const;
    /// Sets the holding process handle in the guest state.
    void SetHoldingThread(SharedPtr<Thread> thread);

    /// Returns the has_waiters bit in the guest state.
    bool GetHasWaiters() const;
    /// Sets the has_waiters bit in the guest state.
    void SetHasWaiters(bool has_waiters);

    /// Flag that indicates that a mutex still has threads waiting for it.
    static constexpr u32 MutexHasWaitersFlag = 0x40000000;
    /// Mask of the bits in a mutex address value that contain the mutex owner.
    static constexpr u32 MutexOwnerMask = 0xBFFFFFFF;

    /// Attempts to acquire a mutex at the specified address.
    static ResultCode TryAcquire(VAddr address, Handle holding_thread_handle,
                                 Handle requesting_thread_handle);

    /// Releases the mutex at the specified address.
    static ResultCode Release(VAddr address);

private:
    Mutex();
    ~Mutex() override;

    /// Object in guest memory used to track the mutex state
    union GuestState {
        u32_le raw;
        /// Handle of the thread that currently holds the mutex, 0 if available
        BitField<0, 30, u32_le> holding_thread_handle;
        /// 1 when there are threads waiting for this mutex, otherwise 0
        BitField<30, 1, u32_le> has_waiters;
    };
    static_assert(sizeof(GuestState) == 4, "GuestState size is incorrect");
};

/**
 * Releases all the mutexes held by the specified thread
 * @param thread Thread that is holding the mutexes
 */
void ReleaseThreadMutexes(Thread* thread);

} // namespace Kernel
