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
    LOG_WARNING(Debug_GPU, "Got Ioctl 0x%x, inputsz: 0x%x, outputsz: 0x%x", command, input.size(),
                output.size());

    switch (command) {
    case IocGetConfigCommand:
        return NvOsGetConfigU32(input, output);
    }
    UNIMPLEMENTED();
    return 0;
}

u32 nvhost_ctrl::NvOsGetConfigU32(const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_WARNING(Service, "(STUBBED) lets do it");
    IocGetConfigParams params;
    std::memcpy(&params, input.data(), sizeof(params));
    LOG_WARNING(Service, "(STUBBED) param_str %s", params.param_str);
    LOG_WARNING(Service, "(STUBBED) domain_str %s", params.domain_str);

    if (!strcmp(params.param_str, "NV_MEMORY_PROFILER")) {
        std::memcpy(&params.config_str, "1\x00", 2);
    } else {
        UNIMPLEMENTED();
    }
    std::memcpy(output.data(), &params, sizeof(params));
    LOG_WARNING(Service, "(STUBBED) memcpy");
    return 0;
}

} // namespace Devices
} // namespace Nvidia
} // namespace Service
