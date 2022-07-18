// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"

namespace Kernel::Arch::Arm64 {

struct KExceptionContext {
    u64 x[(30 - 0) + 1];
    u64 sp;
    u64 pc;
    u32 psr;
    u32 write;
    u64 tpidr;
    u64 reserved;
};
static_assert(sizeof(KExceptionContext) == 0x120);

} // namespace Kernel::Arch::Arm64
