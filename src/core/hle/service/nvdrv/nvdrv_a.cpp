// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/nvdrv/nvdrv_a.h"

namespace Service {
namespace Nvidia {

void NVDRV_A::Open(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    auto buffer = ctx.BufferDescriptorA()[0];

    std::string device_name = Memory::ReadCString(buffer.Address(), buffer.Size());

    u32 fd = nvdrv->Open(device_name);
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

    u32 nv_result = nvdrv->Ioctl(fd, command, input, output);

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

NVDRV_A::NVDRV_A(std::shared_ptr<Module> nvdrv)
    : ServiceFramework("nvdrv:a"), nvdrv(std::move(nvdrv)) {
    static const FunctionInfo functions[] = {
        {0, &NVDRV_A::Open, "Open"},
        {1, &NVDRV_A::Ioctl, "Ioctl"},
        {3, &NVDRV_A::Initialize, "Initialize"},
    };
    RegisterHandlers(functions);
}

} // namespace Nvidia
} // namespace Service
