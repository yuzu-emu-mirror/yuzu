// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/nvdrv/devices/nvhost_ctrl.h"

namespace Service {
namespace Nvidia {
namespace Devices {

u32 nvhost_ctrl::ioctl(Ioctl command, const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "called, command=0x%08x, input_size=0x%zx, output_size=0x%zx",
              command.raw, input.size(), output.size());

    switch (static_cast<IoctlCommand>(command.raw)) {
    case IoctlCommand::IocGetConfigCommand:
        return NvOsGetConfigU32(input, output);
    case IoctlCommand::IocCtrlEventWaitCommand:
        return IocCtrlEventWait(input, output);
    }
    UNIMPLEMENTED();
    return 0;
}

u32 nvhost_ctrl::NvOsGetConfigU32(const std::vector<u8>& input, std::vector<u8>& output) {
    IocGetConfigParams params{};
    std::memcpy(&params, input.data(), sizeof(params));
    LOG_DEBUG(Service_NVDRV, "called, setting=%s!%s", params.domain_str.data(),
              params.param_str.data());

    if (!strcmp(params.domain_str.data(), "nv")) {
        if (!strcmp(params.param_str.data(), "NV_MEMORY_PROFILER")) {
            params.config_str[0] = '0';
        } else if (!strcmp(params.param_str.data(), "NVN_THROUGH_OPENGL")) {
            params.config_str[0] = '0';
        } else if (!strcmp(params.param_str.data(), "NVRM_GPU_PREVENT_USE")) {
            params.config_str[0] = '0';
        } else {
            params.config_str[0] = '0';
        }
    } else {
        UNIMPLEMENTED(); // unknown domain? Only nv has been seen so far on hardware
    }
    std::memcpy(output.data(), &params, sizeof(params));
    return 0;
}

u32 nvhost_ctrl::IocCtrlEventWait(const std::vector<u8>& input, std::vector<u8>& output) {
    IocCtrlEventWaitParams params{};
    std::memcpy(&params, input.data(), sizeof(params));
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, syncpt_id=%u threshold=%u timeout=%d",
                params.syncpt_id, params.threshold, params.timeout);

    // TODO(Subv): Implement actual syncpt waiting.
    params.value = 0;
    std::memcpy(output.data(), &params, sizeof(params));
    return 0;
}

} // namespace Devices
} // namespace Nvidia
} // namespace Service
