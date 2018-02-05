// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <map>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/nvdrv/devices/nvhost_gpu.h"

namespace Service {
namespace Nvidia {
namespace Devices {

u32 nvhost_gpu::ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "Got Ioctl 0x%x, inputsz: 0x%x, outputsz: 0x%x", command, input.size(),
              output.size());

    switch (command) {
    case IocSetNVMAPfdCommand:
        return SetNVMAPfd(input, output);
    case IocSetClientDataCommand:
        return SetClientData(input, output);
    case IocGetClientDataCommand:
        return GetClientData(input, output);
    }
};

u32 nvhost_gpu::SetNVMAPfd(const std::vector<u8>& input, std::vector<u8>& output) {
    set_nvmap_fd params;
    std::memcpy(&params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, fd=%x", params.nvmap_fd);
    nvmap_fd = params.nvmap_fd;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::SetClientData(const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "called");
    client_data params;
    std::memcpy(&params, input.data(), input.size());
    user_data = params.data;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::GetClientData(const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "called");
    client_data params;
    std::memcpy(&params, input.data(), input.size());
    params.data = user_data;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

} // namespace Devices
} // namespace Nvidia
} // namespace Service
