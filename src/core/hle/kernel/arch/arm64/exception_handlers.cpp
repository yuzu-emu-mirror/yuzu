// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/arch/arm64/exception_handlers.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel::Arch::Arm64 {

void HandleException(KernelCore& kernel, const KExceptionContext& context) {
    auto* process = kernel.CurrentProcess();
    if (!process) {
        return;
    }

    const bool is_user_mode = (context.psr & 0xF) == 0;
    if (is_user_mode) {
        // If the user disable count is set, we may need to pin the current thread.
        if (GetCurrentThread(kernel).GetUserDisableCount() != 0 &&
            process->GetPinnedThread(GetCurrentCoreId(kernel)) == nullptr) {
            KScopedSchedulerLock sl{kernel};

            // Pin the current thread.
            process->PinCurrentThread(GetCurrentCoreId(kernel));

            // Set the interrupt flag for the thread.
            GetCurrentThread(kernel).SetInterruptFlag();
        }
    }
}

} // namespace Kernel::Arch::Arm64
