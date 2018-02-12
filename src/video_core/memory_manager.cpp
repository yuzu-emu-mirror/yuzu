// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "video_core/memory_manager.h"

namespace Tegra {

PAddr MemoryManager::AllocateSpace(u64 size, u64 align) {
    boost::optional<PAddr> paddr = FindFreeBlock(size, align);
    ASSERT(paddr);

    for (u64 offset = 0; offset < size; offset += Memory::PAGE_SIZE) {
        PageSlot(*paddr + offset) = static_cast<u64>(PageStatus::Allocated);
    }

    return *paddr;
}

PAddr MemoryManager::AllocateSpace(PAddr paddr, u64 size, u64 align) {
    for (u64 offset = 0; offset < size; offset += Memory::PAGE_SIZE) {
        if (IsPageMapped(paddr + offset)) {
            return AllocateSpace(size, align);
        }
    }

    for (u64 offset = 0; offset < size; offset += Memory::PAGE_SIZE) {
        PageSlot(paddr + offset) = static_cast<u64>(PageStatus::Allocated);
    }

    return paddr;
}

PAddr MemoryManager::MapBufferEx(VAddr vaddr, u64 size) {
    vaddr &= ~Memory::PAGE_MASK;

    boost::optional<PAddr> paddr = FindFreeBlock(size);
    ASSERT(paddr);

    for (u64 offset = 0; offset < size; offset += Memory::PAGE_SIZE) {
        PageSlot(*paddr + offset) = vaddr + offset;
    }

    return *paddr;
}

PAddr MemoryManager::MapBufferEx(VAddr vaddr, PAddr paddr, u64 size) {
    vaddr &= ~Memory::PAGE_MASK;
    paddr &= ~Memory::PAGE_MASK;

    for (u64 offset = 0; offset < size; offset += Memory::PAGE_SIZE) {
        if (PageSlot(paddr + offset) != static_cast<u64>(PageStatus::Allocated)) {
            return MapBufferEx(vaddr, size);
        }
    }

    for (u64 offset = 0; offset < size; offset += Memory::PAGE_SIZE) {
        PageSlot(paddr + offset) = vaddr + offset;
    }

    return paddr;
}

boost::optional<PAddr> MemoryManager::FindFreeBlock(u64 size, u64 align) {
    PAddr paddr{};
    u64 free_space{};
    align = (align + Memory::PAGE_MASK) & ~Memory::PAGE_MASK;

    while (paddr + free_space < MAX_ADDRESS) {
        if (!IsPageMapped(paddr + free_space)) {
            free_space += Memory::PAGE_SIZE;
            if (free_space >= size) {
                return paddr;
            }
        } else {
            paddr += free_space + Memory::PAGE_SIZE;
            free_space = 0;
            const u64 remainder{paddr % align};
            if (!remainder) {
                paddr = (paddr - remainder) + align;
            }
        }
    }

    return {};
}

VAddr MemoryManager::PhysicalToVirtualAddress(PAddr paddr) {
    VAddr base_addr = PageSlot(paddr);
    ASSERT(base_addr != static_cast<u64>(PageStatus::Unmapped));
    return base_addr + (paddr & Memory::PAGE_MASK);
}

bool MemoryManager::IsPageMapped(PAddr paddr) {
    return PageSlot(paddr) != static_cast<u64>(PageStatus::Unmapped);
}

VAddr& MemoryManager::PageSlot(PAddr paddr) {
    auto& block = page_table[(paddr >> (Memory::PAGE_BITS + PAGE_TABLE_BITS)) & PAGE_TABLE_MASK];
    if (!block) {
        block = std::make_unique<PageBlock>();
        for (unsigned index = 0; index < PAGE_BLOCK_SIZE; index++) {
            (*block)[index] = static_cast<u64>(PageStatus::Unmapped);
        }
    }
    return (*block)[(paddr >> Memory::PAGE_BITS) & PAGE_BLOCK_MASK];
}

} // namespace Tegra
