// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/alignment.h"
#include "common/assert.h"
#include "video_core/memory_manager.h"

namespace Tegra {

PAddr MemoryManager::AllocateSpace(u64 size, u64 align) {
    boost::optional<PAddr> paddr = FindFreeBlock(size, align);
    ASSERT(paddr);

    for (u64 offset = 0; offset < size; offset += PAGE_SIZE) {
        ASSERT(PageSlot(*paddr + offset) == static_cast<u64>(PageStatus::Unmapped));
        PageSlot(*paddr + offset) = static_cast<u64>(PageStatus::Allocated);
    }

    return *paddr;
}

PAddr MemoryManager::AllocateSpace(PAddr paddr, u64 size, u64 align) {
    for (u64 offset = 0; offset < size; offset += PAGE_SIZE) {
        ASSERT(PageSlot(paddr + offset) == static_cast<u64>(PageStatus::Unmapped));
        PageSlot(paddr + offset) = static_cast<u64>(PageStatus::Allocated);
    }

    return paddr;
}

PAddr MemoryManager::MapBufferEx(VAddr vaddr, u64 size) {
    boost::optional<PAddr> paddr = FindFreeBlock(size, PAGE_SIZE);
    ASSERT(paddr);

    for (u64 offset = 0; offset < size; offset += PAGE_SIZE) {
        ASSERT(PageSlot(*paddr + offset) == static_cast<u64>(PageStatus::Unmapped));
        PageSlot(*paddr + offset) = vaddr + offset;
    }

    return *paddr;
}

PAddr MemoryManager::MapBufferEx(VAddr vaddr, PAddr paddr, u64 size) {
    ASSERT((paddr & PAGE_MASK) == 0);

    for (u64 offset = 0; offset < size; offset += PAGE_SIZE) {
        ASSERT(PageSlot(paddr + offset) == static_cast<u64>(PageStatus::Allocated));
        PageSlot(paddr + offset) = vaddr + offset;
    }

    return paddr;
}

boost::optional<PAddr> MemoryManager::FindFreeBlock(u64 size, u64 align) {
    PAddr paddr = 0;
    u64 free_space = 0;
    align = (align + PAGE_MASK) & ~PAGE_MASK;

    while (paddr + free_space < MAX_ADDRESS) {
        if (!IsPageMapped(paddr + free_space)) {
            free_space += PAGE_SIZE;
            if (free_space >= size) {
                return paddr;
            }
        } else {
            paddr += free_space + PAGE_SIZE;
            free_space = 0;
            paddr = Common::AlignUp(paddr, align);
        }
    }

    return {};
}

VAddr MemoryManager::PhysicalToVirtualAddress(PAddr paddr) {
    VAddr base_addr = PageSlot(paddr);
    ASSERT(base_addr != static_cast<u64>(PageStatus::Unmapped));
    return base_addr + (paddr & PAGE_MASK);
}

bool MemoryManager::IsPageMapped(PAddr paddr) {
    return PageSlot(paddr) != static_cast<u64>(PageStatus::Unmapped);
}

VAddr& MemoryManager::PageSlot(PAddr paddr) {
    auto& block = page_table[(paddr >> (PAGE_BITS + PAGE_TABLE_BITS)) & PAGE_TABLE_MASK];
    if (!block) {
        block = std::make_unique<PageBlock>();
        for (unsigned index = 0; index < PAGE_BLOCK_SIZE; index++) {
            (*block)[index] = static_cast<u64>(PageStatus::Unmapped);
        }
    }
    return (*block)[(paddr >> PAGE_BITS) & PAGE_BLOCK_MASK];
}

} // namespace Tegra
