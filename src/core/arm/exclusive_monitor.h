// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/common_types.h"

class ExclusiveMonitor {
public:
    virtual ~ExclusiveMonitor();

    virtual void SetExclusive(size_t core_index, u64 addr) = 0;
    virtual void ClearExclusive() = 0;

    virtual bool ExclusiveWrite8(size_t core_index, u64 vaddr, u8 value) = 0;
    virtual bool ExclusiveWrite16(size_t core_index, u64 vaddr, u16 value) = 0;
    virtual bool ExclusiveWrite32(size_t core_index, u64 vaddr, u32 value) = 0;
    virtual bool ExclusiveWrite64(size_t core_index, u64 vaddr, u64 value) = 0;
    virtual bool ExclusiveWrite128(size_t core_index, u64 vaddr,
                                   std::array<std::uint64_t, 2> value) = 0;
};
