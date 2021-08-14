// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/device_memory.h"

namespace Core {

DeviceMemory::DeviceMemory() : buffer{DramMemoryMap::Size, 1ULL << 39} {
    auto ptr = reinterpret_cast<std::size_t>(buffer.BackingBasePointer());
    if (ptr & 0xfff || !ptr) {
        LOG_CRITICAL(HW_Memory, "Unaligned DeviceMemory");
        abort();
    }
}

DeviceMemory::~DeviceMemory() = default;

} // namespace Core
