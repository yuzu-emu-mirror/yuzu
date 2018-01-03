// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

namespace Kernel {

struct MemoryInfo {
    u64 base_address;
    u64 size;
    u32 type;
    u32 attributes;
    u32 permission;
};

struct PageInfo {
    u64 flags;
};

/// Values accepted by svcGetInfo
enum class GetInfoType : u64 {
    // 1.0.0+
    TotalMemoryUsage = 6,
    TotalHeapUsage = 7,
    RandomEntropy = 11,
    // 2.0.0+
    AddressSpaceBaseAddr = 12,
    AddressSpaceSize = 13,
    NewMapRegionBaseAddr = 14,
    NewMapRegionSize = 15,
};

void CallSVC(u32 immediate);

} // namespace Kernel
