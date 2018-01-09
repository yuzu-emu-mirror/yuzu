// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>
#include "common/common_types.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"

namespace Service {
namespace NVDRV {
namespace Devices {

class nvmap;

class nvdisp_disp0 final : public nvdevice {
public:
    nvdisp_disp0(std::shared_ptr<nvmap> nvmap_dev) : nvdevice(), nvmap_dev(std::move(nvmap_dev)) {}
    ~nvdisp_disp0() = default;

    u32 ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) override;

    /// Performs a screen flip, drawing the buffer pointed to by the handle.
    void flip(u32 buffer_handle, u32 offset, u32 format, u32 width, u32 height, u32 stride);

private:
    std::shared_ptr<nvmap> nvmap_dev;
};

} // namespace Devices
} // namespace NVDRV
} // namespace Service
