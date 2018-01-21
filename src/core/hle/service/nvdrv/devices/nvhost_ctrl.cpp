// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/nvdrv/devices/nvhost_ctrl.h"

namespace Service {
namespace Nvidia {
namespace Devices {

u32 nvhost_ctrl::ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "called, command=0x%08x, input_size=0x%lx, output_size=0x%lx", command,
              input.size(), output.size());

    switch (command) {
    case IocGetConfigCommand:
        return NvOsGetConfigU32(input, output);
    }
    UNIMPLEMENTED();
    return 0;
}

u32 nvhost_ctrl::NvOsGetConfigU32(const std::vector<u8>& input, std::vector<u8>& output) {
    IocGetConfigParams params;
    std::memcpy(&params, input.data(), sizeof(params));
    LOG_DEBUG(Service_NVDRV, "called, setting=%s!%s", params.domain_str.data(),
              params.param_str.data());

    if (!strcmp(params.domain_str.data(), "nv")) {
        if (!strcmp(params.param_str.data(), "NV_MEMORY_PROFILER")) {
            params.config_str[0] = '1';
        } else {
            UNIMPLEMENTED();
        }
    } else {
        UNIMPLEMENTED();
    }
    std::memcpy(output.data(), &params, sizeof(params));
    return 0;
}

} // namespace Devices
} // namespace Nvidia
} // namespace Service
