// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <mutex>
#include <tuple>

#include "common/common_types.h"
#include "common/memory_hook.h"
#include "common/virtual_buffer.h"

namespace Common {

enum class PageType : u8 {
    /// Page is unmapped and should cause an access error.
    Unmapped,
    /// Page is mapped to regular memory. This is the only type you can get pointers to.
    Memory,
    /// Page is mapped to regular memory, but also needs to check for rasterizer cache flushing and
    /// invalidation
    RasterizerCachedMemory,
    /// Page is mapped to a I/O region. Writing and reading to this page is handled by functions.
    Special,
    /// Page is allocated for use.
    Allocated,
};

struct SpecialRegion {
    enum class Type {
        DebugHook,
        IODevice,
    } type;

    MemoryHookPointer handler;

    [[nodiscard]] bool operator<(const SpecialRegion& other) const {
        return std::tie(type, handler) < std::tie(other.type, other.handler);
    }

    [[nodiscard]] bool operator==(const SpecialRegion& other) const {
        return std::tie(type, handler) == std::tie(other.type, other.handler);
    }
};

/**
 * A (reasonably) fast way of allowing switchable and remappable process address spaces. It loosely
 * mimics the way a real CPU page table works.
 */
class PageTable {
public:
    PageTable();
    ~PageTable() noexcept;

    PageTable(const PageTable&) = delete;
    PageTable& operator=(const PageTable&) = delete;

    PageTable(PageTable&&) noexcept = default;
    PageTable& operator=(PageTable&&) noexcept = default;

    /**
     * Resizes the page table to be able to accomodate enough pages within
     * a given address space.
     *
     * @param address_space_width_in_bits The address size width in bits.
     * @param page_size_in_bits The page size in bits.
     */
    void Resize(std::size_t address_space_width_in_bits, std::size_t page_size_in_bits);

    struct Entry {
        u8* const pointer;
        const u64 backing_addr;
        const Common::PageType attribute;
    };

    VirtualBuffer<u8*>& GetPointers() {
        return pointers;
    }

    u64 GetBackingAddr(std::size_t base) {
        std::lock_guard<std::mutex> lock(guard);
        return backing_addr[base];
    }

    Entry GetEntry(std::size_t base) {
        std::lock_guard<std::mutex> lock(guard);
        return {
            .pointer = pointers[base],
            .backing_addr = backing_addr[base],
            .attribute = attributes[base],
        };
    }

    void SetEntry(std::size_t base, const Entry& entry) {
        std::lock_guard<std::mutex> lock(guard);
        pointers[base] = entry.pointer;
        backing_addr[base] = entry.backing_addr;
        attributes[base] = entry.attribute;
    }

private:
    /**
     * Vector of memory pointers backing each page. An entry can only be non-null if the
     * corresponding entry in the `attributes` vector is of type `Memory`.
     */
    VirtualBuffer<u8*> pointers;

    VirtualBuffer<u64> backing_addr;

    VirtualBuffer<PageType> attributes;

    std::mutex guard;
};

} // namespace Common
