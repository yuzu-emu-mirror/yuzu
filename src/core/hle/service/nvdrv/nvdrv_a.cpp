// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"
#include "core/hle/service/nvdrv/devices/nvdisp_disp0.h"
#include "core/hle/service/nvdrv/devices/nvhost_as_gpu.h"
#include "core/hle/service/nvdrv/devices/nvmap.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/nvdrv/nvdrv_a.h"

namespace Service {
namespace NVDRV {

void NVDRV_A::Open(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    auto buffer = ctx.BufferDescriptorA()[0];

    std::string device_name = Memory::ReadCString(buffer.Address(), buffer.Size());

    auto device = devices[device_name];
    u32 fd = next_fd++;

    open_files[fd] = device;

    IPC::RequestBuilder rb{ctx, 4};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(fd);
    rb.Push<u32>(0);
}

void NVDRV_A::Ioctl(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::RequestParser rp{ctx};
    u32 fd = rp.Pop<u32>();
    u32 command = rp.Pop<u32>();

    auto input_buffer = ctx.BufferDescriptorA()[0];
    auto output_buffer = ctx.BufferDescriptorB()[0];

    std::vector<u8> input(input_buffer.Size());
    std::vector<u8> output(output_buffer.Size());

    Memory::ReadBlock(input_buffer.Address(), input.data(), input_buffer.Size());
    auto itr = open_files.find(fd);
    ASSERT_MSG(itr != open_files.end(), "Tried to talk to an invalid device");

    auto device = itr->second;
    u32 nv_result = device->ioctl(command, input, output);

    Memory::WriteBlock(output_buffer.Address(), output.data(), output_buffer.Size());

    IPC::RequestBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push(nv_result);
}

void NVDRV_A::Initialize(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::RequestBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0);
}

NVDRV_A::NVDRV_A() : ServiceFramework("nvdrv:a") {
    static const FunctionInfo functions[] = {
        {0, &NVDRV_A::Open, "Open"},
        {1, &NVDRV_A::Ioctl, "Ioctl"},
        {3, &NVDRV_A::Initialize, "Initialize"},
    };
    RegisterHandlers(functions);

    auto nvmap_dev = std::make_shared<Devices::nvmap>();
    devices["/dev/nvhost-as-gpu"] = std::make_shared<Devices::nvhost_as_gpu>();
    devices["/dev/nvmap"] = nvmap_dev;
    devices["/dev/nvdisp_disp0"] = std::make_shared<Devices::nvdisp_disp0>(nvmap_dev);
}

} // namespace NVDRV
} // namespace Service
