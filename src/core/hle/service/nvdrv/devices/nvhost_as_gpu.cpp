// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/service/nvdrv/devices/nvhost_as_gpu.h"
#include "core/hle/service/nvdrv/devices/nvmap.h"

namespace Service {
namespace Nvidia {
namespace Devices {

u32 nvhost_as_gpu::ioctl(Ioctl command, const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "called, command=0x%08x, input_size=0x%zx, output_size=0x%zx",
              command.raw, input.size(), output.size());

    switch (static_cast<IoctlCommand>(command.raw)) {
    case IoctlCommand::IocInitalizeExCommand:
        return InitalizeEx(input, output);
    case IoctlCommand::IocAllocateSpaceCommand:
        return AllocateSpace(input, output);
    case IoctlCommand::IocMapBufferExCommand:
        return MapBufferEx(input, output);
    case IoctlCommand::IocBindChannelCommand:
        return BindChannel(input, output);
    case IoctlCommand::IocGetVaRegionsCommand:
        return GetVARegions(input, output);
    }
    return 0;
}

u32 nvhost_as_gpu::InitalizeEx(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlInitalizeEx params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, big_page_size=0x%x", params.big_page_size);
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_as_gpu::AllocateSpace(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlAllocSpace params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, pages=%x, page_size=%x, flags=%x", params.pages,
              params.page_size, params.flags);

    auto& gpu = Core::System::GetInstance().GPU();
    const u64 size{static_cast<u64>(params.pages) * static_cast<u64>(params.page_size)};
    if (params.flags & 1) {
        params.offset = gpu.memory_manager->AllocateSpace(params.offset, size, 1);
    } else {
        params.offset = gpu.memory_manager->AllocateSpace(size, params.align);
    }

    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_as_gpu::MapBufferEx(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlMapBufferEx params{};
    std::memcpy(&params, input.data(), input.size());

    LOG_DEBUG(Service_NVDRV,
              "called, flags=%x, nvmap_handle=%x, buffer_offset=%" PRIu64 ", mapping_size=%" PRIu64
              ", offset=%" PRIu64,
              params.flags, params.nvmap_handle, params.buffer_offset, params.mapping_size,
              params.offset);

    if (!params.nvmap_handle) {
        return 0;
    }

    auto object = nvmap_dev->GetObject(params.nvmap_handle);
    ASSERT(object);

    auto& gpu = Core::System::GetInstance().GPU();

    if (params.flags & 1) {
        params.offset = gpu.memory_manager->MapBufferEx(object->addr, params.offset, object->size);
    } else {
        params.offset = gpu.memory_manager->MapBufferEx(object->addr, object->size);
    }

    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_as_gpu::BindChannel(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlBindChannel params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, fd=%x", params.fd);
    channel = params.fd;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_as_gpu::GetVARegions(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlGetVaRegions params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, buf_addr=%" PRIu64 ", buf_size=%x",
                params.buf_addr, params.buf_size);

    params.buf_size = 0x30;
    params.regions[0].offset = 0x04000000;
    params.regions[0].page_size = 0x1000;
    params.regions[0].pages = 0x3fbfff;

    params.regions[1].offset = 0x04000000;
    params.regions[1].page_size = 0x10000;
    params.regions[1].pages = 0x1bffff;
    // TODO(ogniK): This probably can stay stubbed but should add support way way later
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

} // namespace Devices
} // namespace Nvidia
} // namespace Service
