// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/thread.h"
#include "core/memory.h"

namespace Kernel {
    namespace AddressArbiter {

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
            // TODO
            return RESULT_SUCCESS;
        }

        // Waits on an address if the value passed is equal to the argument value.
        ResultCode WaitForAddressIfEqual(VAddr address, s32 value, s64 timeout) {
            // TODO
            return RESULT_SUCCESS;
        }
    } // namespace AddressArbiter
} // namespace Kernel