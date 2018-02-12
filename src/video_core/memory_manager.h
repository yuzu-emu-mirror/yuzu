// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>
#include "common/common_types.h"
#include "core/memory.h"

namespace Tegra {

/// Virtual addresses in the GPU's memory map are 64 bit.
using GPUVAddr = u64;

class MemoryManager final {
public:
    MemoryManager() = default;

    PAddr AllocateSpace(u64 size, u64 align);
    PAddr AllocateSpace(PAddr paddr, u64 size, u64 align);
    PAddr MapBufferEx(VAddr vaddr, u64 size);
    PAddr MapBufferEx(VAddr vaddr, PAddr paddr, u64 size);
    VAddr PhysicalToVirtualAddress(PAddr paddr);

private:
    boost::optional<PAddr> FindFreeBlock(u64 size, u64 align = 1);
    bool IsPageMapped(PAddr paddr);
    VAddr& PageSlot(PAddr paddr);

    enum class PageStatus : u64 {
        Unmapped = 0xFFFFFFFFFFFFFFFFULL,
        Allocated = 0xFFFFFFFFFFFFFFFEULL,
    };

    static constexpr u64 MAX_ADDRESS{0x10000000000ULL};
    static constexpr u64 PAGE_TABLE_BITS{14};
    static constexpr u64 PAGE_TABLE_SIZE{1 << PAGE_TABLE_BITS};
    static constexpr u64 PAGE_TABLE_MASK{PAGE_TABLE_SIZE - 1};
    static constexpr u64 PAGE_BLOCK_BITS{14};
    static constexpr u64 PAGE_BLOCK_SIZE{1 << PAGE_BLOCK_BITS};
    static constexpr u64 PAGE_BLOCK_MASK{PAGE_BLOCK_SIZE - 1};

    using PageBlock = std::array<VAddr, PAGE_BLOCK_SIZE>;
    std::array<std::unique_ptr<PageBlock>, PAGE_TABLE_SIZE> page_table{};
};

} // namespace Tegra
