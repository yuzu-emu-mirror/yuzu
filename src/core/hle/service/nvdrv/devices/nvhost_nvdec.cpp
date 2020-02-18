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
constexpr u32 NVHOST_IOCTL_CHANNEL_MAP_CMD_BUFFER_EX(0x25);
constexpr u32 NVHOST_IOCTL_CHANNEL_SUBMIT(0x1);

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
    case IoctlCommand::IocChannelGetWaitBase:
        return ChannelGetWaitBase(input, output);
    case IoctlCommand::IocChannelGetSyncPoint:
        return ChannelGetSyncPoint(input, output);
    }

    if (command.group == NVHOST_IOCTL_MAGIC) {
        if (command.cmd == NVHOST_IOCTL_CHANNEL_MAP_CMD_BUFFER) {
            return ChannelMapCmdBuffer(input, output);
        } else if (command.cmd == NVHOST_IOCTL_CHANNEL_SUBMIT) {
            return ChannelSubmit(input, output);
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

u32 nvhost_nvdec::ChannelSubmit(const std::vector<u8>& input, std::vector<u8>& output) {
    u32 startPt = 16;

    IoctlSubmit params{};
    std::memcpy(&params, input.data(), startPt);

    std::vector<IoctlCmdBuf> cmdBufs(params.num_cmdbufs);
    std::memcpy(cmdBufs.data(), input.data() + startPt,
                sizeof(IoctlCmdBuf) * params.num_cmdbufs);
    startPt += sizeof(IoctlCmdBuf) * params.num_cmdbufs;
    // TODO(namkazt): what to do with this?

    std::vector<IoctlReloc> relocs(params.num_relocs);
    std::memcpy(relocs.data(), input.data() + startPt, sizeof(IoctlReloc) * params.num_relocs);
    startPt += sizeof(IoctlReloc) * params.num_relocs;
    // TODO(namkazt): what to do with this?

    std::vector<IoctlRelocShift> relocShifts(params.num_relocs);
    std::memcpy(relocShifts.data(), input.data() + startPt,
                sizeof(IoctlRelocShift) * params.num_relocs);
    startPt += sizeof(IoctlRelocShift) * params.num_relocs;
    //TODO(namkazt): what to do with this?

    std::vector<IoctlSyncPtIncr> syncPtIncrs(params.num_syncpt_incrs);
    std::memcpy(syncPtIncrs.data(), input.data() + startPt,
                sizeof(IoctlSyncPtIncr) * params.num_syncpt_incrs);
    startPt += sizeof(IoctlSyncPtIncr) * params.num_syncpt_incrs;
    // apply increment to sync points and create new one if not existed
    for (auto syncIncr : syncPtIncrs) {
        auto itr = syncPtValues.find(syncIncr.syncpt_id);
        if (itr == syncPtValues.end()) {
            syncPtValues[syncIncr.syncpt_id] = syncIncr.syncpt_incrs;
        } else {
            syncPtValues[syncIncr.syncpt_id] += syncIncr.syncpt_incrs;
        }
    }

    LOG_WARNING(Service_NVDRV,
        "(STUBBED) called, num_cmdbufs: {}, num_relocs: {}, num_syncpt_incrs: {}, num_fences: {}", 
        params.num_cmdbufs, params.num_relocs, params.num_syncpt_incrs, params.num_fences);

    std::memcpy(output.data(), &params, 16);
    return 0;
}

u32 nvhost_nvdec::ChannelGetSyncPoint(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctChannelSyncPoint params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "called, module_id: {}", params.syncpt_id);

    auto itr = syncPtValues.find(params.syncpt_id);
    if (itr == syncPtValues.end()) {
        params.syncpt_value = 0;
    } else {
        params.syncpt_value = itr->second;
    }

    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_nvdec::ChannelGetWaitBase(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctChannelWaitBase params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, module_id: {}", params.module_id);

    params.waitbase_value = 0;

    std::memcpy(output.data(), &params, output.size());
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
