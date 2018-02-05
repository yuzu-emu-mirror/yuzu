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
    case IocZCullBind:
        return ZCullBind(input, output);
    case IocSetErrorNotifierCommand:
        return SetErrorNotifier(input, output);
    case IocChannelSetPriorityCommand:
        return SetChannelPriority(input, output);
    case IocAllocGPFIFOEx2Command:
        return AllocGPFIFOEx2(input, output);
    case IocAllocObjCtxCommand:
        return AllocateObjectContext(input, output);
    }

    // Variable size ioctls
    u8 group = (command >> 8) & 0xff;
    u8 index = command & 0xff;

    if (group == 'H') {
        if (index == 0x8) {
            return SubmitGPFIFO(input, output);
        }
    }

    UNIMPLEMENTED();
    return 0;
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

u32 nvhost_gpu::ZCullBind(const std::vector<u8>& input, std::vector<u8>& output) {
    std::memcpy(&zcull_params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, gpu_va=%lx, mode=%x", zcull_params.gpu_va, zcull_params.mode);
    std::memcpy(output.data(), &zcull_params, output.size());
    return 0;
}

u32 nvhost_gpu::SetErrorNotifier(const std::vector<u8>& input, std::vector<u8>& output) {
    set_error_notifier params;
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, offset=%lx, size=%lx, mem=%x", params.offset,
                params.size, params.mem);
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::SetChannelPriority(const std::vector<u8>& input, std::vector<u8>& output) {
    std::memcpy(&channel_priority, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "(STUBBED) called, priority=%x", channel_priority);
    std::memcpy(output.data(), &channel_priority, output.size());
    return 0;
}

u32 nvhost_gpu::AllocGPFIFOEx2(const std::vector<u8>& input, std::vector<u8>& output) {
    alloc_gpfifo_ex2 params;
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV,
                "(STUBBED) called, num_entries=%x, flags=%x, unk0=%x, unk1=%x, unk2=%x, unk3=%x",
                params.__num_entries, params.__flags, params.__unk0, params.__unk1, params.__unk2,
                params.__unk3);
    params.__fence_out.id = 0;
    params.__fence_out.value = 0;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::AllocateObjectContext(const std::vector<u8>& input, std::vector<u8>& output) {
    alloc_obj_ctx params;
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, class_num=%x, flags=%x", params.class_num,
                params.flags);
    params.obj_id = 0x0;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::SubmitGPFIFO(const std::vector<u8>& input, std::vector<u8>& output) {
    submit_gpfifo params;
    if (input.size() < 24)
        UNIMPLEMENTED();
    std::memcpy(&params, input.data(), 24);
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, gpfifo=%lx, num_entries=%x, flags=%x",
                params.gpfifo, params.num_entries, params.flags);

    gpfifo_entry* entries = new gpfifo_entry[params.num_entries];
    std::memcpy(entries, &input.data()[24], params.num_entries * 8);
    for (int i = 0; i < params.num_entries; i++) {
        u32 unk1 = (entries[i].entry1 >> 8) & 0x3;
        u32 sz = (entries[i].entry1 >> 10) & 0x1fffff;
        u32 unk2 = entries[i].entry1 >> 31;
        u64 va_addr = entries[i].entry0 | ((entries[i].entry1 & 0xff) << 32);
        // TODO(ogniK): Process these
    }
    params.fence_out.id = 0;
    params.fence_out.value = 0;
    std::memcpy(output.data(), &params, output.size());
    delete[] entries;
    return 0;
}

} // namespace Devices
} // namespace Nvidia
} // namespace Service
