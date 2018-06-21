// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/core.h"
#include "core/hle/kernel/errors.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/lock.h"
#include "core/memory.h"

namespace Kernel {
    namespace AddressArbiter {

        // Performs actual address waiting logic.
        ResultCode WaitForAddress(VAddr address, s64 timeout) {
            SharedPtr<Thread> current_thread = GetCurrentThread();
            current_thread->arb_wait_address = address;
            current_thread->arb_wait_result = RESULT_TIMEOUT;
            current_thread->status = THREADSTATUS_WAIT_ARB;
            current_thread->wakeup_callback = nullptr;

            current_thread->WakeAfterDelay(timeout);

            Core::System::GetInstance().CpuCore(current_thread->processor_id).PrepareReschedule();
            return RESULT_SUCCESS;
        }

        // Signals an address being waited on.
        ResultCode SignalToAddress(VAddr address, s32 value, s32 num_to_wake) {
            // TODO
            return RESULT_SUCCESS;
        }

        // Signals an address being waited on and increments its value if equal to the value argument.
        ResultCode IncrementAndSignalToAddressIfEqual(VAddr address, s32 value, s32 num_to_wake) {
            // TODO
            return RESULT_SUCCESS;
        }

        // Signals an address being waited on and modifies its value based on waiting thread count if equal to the value argument.
        ResultCode ModifyByWaitingCountAndSignalToAddressIfEqual(VAddr address, s32 value, s32 num_to_wake) {
            // TODO
            return RESULT_SUCCESS;
        }

        // Waits on an address if the value passed is less than the argument value, optionally decrementing.
        ResultCode WaitForAddressIfLessThan(VAddr address, s32 value, s64 timeout, bool should_decrement) {
            // Ensure that we can read the address.
            if (!Memory::IsValidVirtualAddress(address)) {
                return ERR_INVALID_ADDRESS_STATE;
            }

            s32 cur_value;
            // Get value, decrementing if less than
            {
                // Decrement if less than must be an atomic operation.
                std::lock_guard<std::recursive_mutex> lock(HLE::g_hle_lock);
                cur_value = (s32)Memory::Read32(address);
                if (cur_value < value) {
                    Memory::Write32(address, (u32)(cur_value - 1));
                }
            }
            if (cur_value >= value) {
                return ERR_INVALID_STATE;
            }
            // Short-circuit without rescheduling, if timeout is zero.
            if (timeout == 0) {
                return RESULT_TIMEOUT;
            }

            return WaitForAddress(address, timeout);
        }

        // Waits on an address if the value passed is equal to the argument value.
        ResultCode WaitForAddressIfEqual(VAddr address, s32 value, s64 timeout) {
            // Ensure that we can read the address.
            if (!Memory::IsValidVirtualAddress(address)) {
                return ERR_INVALID_ADDRESS_STATE;
            }
            // Only wait for the address if equal.
            if ((s32)Memory::Read32(address) != value) {
                return ERR_INVALID_STATE;
            }
            // Short-circuit without rescheduling, if timeout is zero.
            if (timeout == 0) {
                return RESULT_TIMEOUT;
            }

            return WaitForAddress(address, timeout);
        }
    } // namespace AddressArbiter
} // namespace Kernel