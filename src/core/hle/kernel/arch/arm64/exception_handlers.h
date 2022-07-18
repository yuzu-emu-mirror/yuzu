// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hle/kernel/arch/arm64/k_exception_context.h"

namespace Kernel {
class KernelCore;
}

namespace Kernel::Arch::Arm64 {
void HandleException(KernelCore& kernel, const KExceptionContext& context);
}
