// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "common/common_types.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"

namespace Service {
namespace NVDRV {
namespace Devices {

class nvhost_as_gpu final : public nvdevice {
public:
    nvhost_as_gpu() = default;
    ~nvhost_as_gpu() override = default;

    u32 ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) override;
};

} // namespace Devices
} // namespace NVDRV
} // namespace Service
