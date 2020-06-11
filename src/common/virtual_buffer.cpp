// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#ifdef _WIN32
#include <windows.h>
#else
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#if defined __APPLE__ || defined __FreeBSD__ || defined __OpenBSD__
#include <sys/sysctl.h>
#elif defined __HAIKU__
#include <OS.h>
#else
#include <sys/sysinfo.h>
#endif
#endif

#include "common/assert.h"
#include "common/virtual_buffer.h"

namespace Common {

const VAddr address_space_size{1ULL << 39};
void* vmem{nullptr};
void* next_base{nullptr};

void* AllocateMemoryPages(std::size_t size) {

#ifdef _WIN32
    if (vmem == nullptr) {
        vmem = VirtualAlloc(nullptr, address_space_size, MEM_RESERVE, PAGE_READWRITE);
        ASSERT(vmem);
        next_base = vmem;
    }
    void* base{VirtualAlloc(next_base, size, MEM_COMMIT, PAGE_READWRITE)};
    next_base = (char*)base + size;
#else
    void* base{mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0)};

    if (base == MAP_FAILED) {
        base = nullptr;
    }
#endif

    ASSERT(base);
    ASSERT(base >= vmem);
    ASSERT(base < (char*)vmem + address_space_size);

    return base;
}

void FreeMemoryPages(void* base, std::size_t size) {
    if (!base) {
        return;
    }
#ifdef _WIN32
    ASSERT(VirtualFree(base, size, MEM_DECOMMIT));
#else
    ASSERT(munmap(base, size) == 0);
#endif
}

} // namespace Common
