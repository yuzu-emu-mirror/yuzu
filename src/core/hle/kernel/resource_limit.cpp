// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/kernel/resource_limit.h"

namespace Kernel {

static SharedPtr<ResourceLimit> resource_limits[4];

ResourceLimit::ResourceLimit() {}
ResourceLimit::~ResourceLimit() {}

SharedPtr<ResourceLimit> ResourceLimit::Create(std::string name) {
    SharedPtr<ResourceLimit> resource_limit(new ResourceLimit);

    resource_limit->name = std::move(name);
    return resource_limit;
}

SharedPtr<ResourceLimit> ResourceLimit::GetForCategory(ResourceLimitCategory category) {
    switch (category) {
    case ResourceLimitCategory::APPLICATION:
    case ResourceLimitCategory::SYS_APPLET:
    case ResourceLimitCategory::LIB_APPLET:
    case ResourceLimitCategory::OTHER:
        return resource_limits[static_cast<u8>(category)];
    default:
        LOG_CRITICAL(Kernel, "Unknown resource limit category");
        UNREACHABLE();
    }
}

s32 ResourceLimit::GetCurrentResourceValue(ResourceType resource) const {
    switch (resource) {
    case ResourceType::Commit:
        return current_commit;
    case ResourceType::Thread:
        return current_threads;
    case ResourceType::Event:
        return current_events;
    case ResourceType::Mutex:
        return current_mutexes;
    case ResourceType::Semaphore:
        return current_semaphores;
    case ResourceType::Timer:
        return current_timers;
    case ResourceType::SharedMemory:
        return current_shared_mems;
    case ResourceType::AddressArbiter:
        return current_address_arbiters;
    case ResourceType::CPUTime:
        return current_cpu_time;
    default:
        LOG_ERROR(Kernel, "Unknown resource type=%08X", static_cast<u32>(resource));
        UNIMPLEMENTED();
        return 0;
    }
}

u32 ResourceLimit::GetMaxResourceValue(ResourceType resource) const {
    switch (resource) {
    case ResourceType::Priority:
        return max_priority;
    case ResourceType::Commit:
        return max_commit;
    case ResourceType::Thread:
        return max_threads;
    case ResourceType::Event:
        return max_events;
    case ResourceType::Mutex:
        return max_mutexes;
    case ResourceType::Semaphore:
        return max_semaphores;
    case ResourceType::Timer:
        return max_timers;
    case ResourceType::SharedMemory:
        return max_shared_mems;
    case ResourceType::AddressArbiter:
        return max_address_arbiters;
    case ResourceType::CPUTime:
        return max_cpu_time;
    default:
        LOG_ERROR(Kernel, "Unknown resource type=%08X", static_cast<u32>(resource));
        UNIMPLEMENTED();
        return 0;
    }
}

void ResourceLimitsInit() {
    // Create the four resource limits that the system uses
    // Create the APPLICATION resource limit
    SharedPtr<ResourceLimit> resource_limit = ResourceLimit::Create("Applications");
    resource_limit->max_priority = 0x18;
    resource_limit->max_commit = 0x4000000;
    resource_limit->max_threads = 0x20;
    resource_limit->max_events = 0x20;
    resource_limit->max_mutexes = 0x20;
    resource_limit->max_semaphores = 0x8;
    resource_limit->max_timers = 0x8;
    resource_limit->max_shared_mems = 0x10;
    resource_limit->max_address_arbiters = 0x2;
    resource_limit->max_cpu_time = 0x1E;
    resource_limits[static_cast<u8>(ResourceLimitCategory::APPLICATION)] = resource_limit;

    // Create the SYS_APPLET resource limit
    resource_limit = ResourceLimit::Create("System Applets");
    resource_limit->max_priority = 0x4;
    resource_limit->max_commit = 0x5E00000;
    resource_limit->max_threads = 0x1D;
    resource_limit->max_events = 0xB;
    resource_limit->max_mutexes = 0x8;
    resource_limit->max_semaphores = 0x4;
    resource_limit->max_timers = 0x4;
    resource_limit->max_shared_mems = 0x8;
    resource_limit->max_address_arbiters = 0x3;
    resource_limit->max_cpu_time = 0x2710;
    resource_limits[static_cast<u8>(ResourceLimitCategory::SYS_APPLET)] = resource_limit;

    // Create the LIB_APPLET resource limit
    resource_limit = ResourceLimit::Create("Library Applets");
    resource_limit->max_priority = 0x4;
    resource_limit->max_commit = 0x600000;
    resource_limit->max_threads = 0xE;
    resource_limit->max_events = 0x8;
    resource_limit->max_mutexes = 0x8;
    resource_limit->max_semaphores = 0x4;
    resource_limit->max_timers = 0x4;
    resource_limit->max_shared_mems = 0x8;
    resource_limit->max_address_arbiters = 0x1;
    resource_limit->max_cpu_time = 0x2710;
    resource_limits[static_cast<u8>(ResourceLimitCategory::LIB_APPLET)] = resource_limit;

    // Create the OTHER resource limit
    resource_limit = ResourceLimit::Create("Others");
    resource_limit->max_priority = 0x4;
    resource_limit->max_commit = 0x2180000;
    resource_limit->max_threads = 0xE1;
    resource_limit->max_events = 0x108;
    resource_limit->max_mutexes = 0x25;
    resource_limit->max_semaphores = 0x43;
    resource_limit->max_timers = 0x2C;
    resource_limit->max_shared_mems = 0x1F;
    resource_limit->max_address_arbiters = 0x2D;
    resource_limit->max_cpu_time = 0x3E8;
    resource_limits[static_cast<u8>(ResourceLimitCategory::OTHER)] = resource_limit;
}

void ResourceLimitsShutdown() {}

} // namespace Kernel
