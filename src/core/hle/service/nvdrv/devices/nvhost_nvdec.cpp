// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/nvdrv/devices/nvhost_nvdec.h"

namespace Service::Nvidia::Devices {

constexpr u32 NVHOST_IOCTL_MAGIC(0x0);
constexpr u32 NVHOST_IOCTL_CHANNEL_MAP_CMD_BUFFER(0x9);
  
nvhost_nvdec::nvhost_nvdec(Core::System& system) : nvdevice(system) {}
nvhost_nvdec::~nvhost_nvdec() = default;

u32 nvhost_nvdec::ioctl(Ioctl command, const std::vector<u8>& input, const std::vector<u8>& input2,
                        std::vector<u8>& output, std::vector<u8>& output2, IoctlCtrl& ctrl,
                        IoctlVersion version) {
    LOG_DEBUG(Service_NVDRV, "called, command=0x{:08X}, input_size=0x{:X}, output_size=0x{:X}",
              command.raw, input.size(), output.size());

    switch (static_cast<IoctlCommand>(command.raw)) {
    case IoctlCommand::IocSetNVMAPfdCommand:
        return SetNVMAPfd(input, output);
    }
  
    if (command.group == NVHOST_IOCTL_MAGIC) {
        if (command.cmd == NVHOST_IOCTL_CHANNEL_MAP_CMD_BUFFER) {
            return ChannelMapCmdBuffer(input, output);
        }
    }

    UNIMPLEMENTED_MSG("Unimplemented ioctl");
    return 0;
}

u32 nvhost_nvdec::SetNVMAPfd(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlSetNvmapFD params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, fd={}", params.nvmap_fd);

    nvmap_fd = params.nvmap_fd;
    return 0;
}

u32 nvhost_nvdec::ChannelMapCmdBuffer(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlMapCmdBuffer params{};
    std::memcpy(&params, input.data(), 12);

    std::vector<IoctlHandleMapBuffer> handles(params.num_handles);
    std::memcpy(handles.data(), input.data() + 12,
                sizeof(IoctlHandleMapBuffer) * params.num_handles);

    LOG_WARNING(Service_NVDRV, "(STUBBED) called, num_handles: {}, is_compressed: {}", params.num_handles, params.is_compressed);

    //TODO(namkazt): Uses nvmap_pin internally to pin a given number of nvmap handles to an appropriate device
    // physical address.

    std::memcpy(output.data(), &params, 12);
    std::memcpy(output.data() + 12, handles.data(),
                sizeof(IoctlHandleMapBuffer) * params.num_handles);
    return 0;
}
  
} // namespace Service::Nvidia::Devices
