// Copyright 2018 Yuzu Emulator Team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/nvdrv/devices/nvdisp_disp0.h"
#include "core/hle/service/nvdrv/devices/nvmap.h"

namespace Service {
namespace NVDRV {
namespace Devices {

u32 nvdisp_disp0::ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) {
    ASSERT(false, "Unimplemented");
    return 0;
}

void nvdisp_disp0::flip(u32 buffer_handle, u32 offset, u32 format, u32 width, u32 height,
                        u32 stride) {
    VAddr addr = nvmap_dev->GetObjectAddress(buffer_handle);
    LOG_WARNING(Service,
                "Drawing from address %llx offset %08X Width %u Height %u Stride %u Format %u",
                addr, offset, width, height, stride, format);
}

} // namespace Devices
} // namespace NVDRV
} // namespace Service
