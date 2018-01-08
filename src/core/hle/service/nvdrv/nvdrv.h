// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "common/common_types.h"
#include "core/hle/service/service.h"

namespace Service {
namespace NVDRV {

class nvdevice {
public:
    virtual ~nvdevice() = default;

    virtual u32 ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) = 0;
};

/// Registers all NVDRV services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace NVDRV
} // namespace Service
