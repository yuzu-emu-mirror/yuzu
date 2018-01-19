// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nvdrv/interface.h"
#include "core/hle/service/nvdrv/nvdrv.h"

namespace Service {
namespace Nvidia {

void NVDRV::Open(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    auto buffer = ctx.BufferDescriptorA()[0];

    std::string device_name = Memory::ReadCString(buffer.Address(), buffer.Size());

    u32 fd = nvdrv->Open(device_name);
    IPC::RequestBuilder rb{ctx, 4};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(fd);
    rb.Push<u32>(0);
}

void NVDRV::Ioctl(Kernel::HLERequestContext& ctx) {
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

void NVDRV::Close(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::RequestParser rp{ctx};
    u32 fd = rp.Pop<u32>();

    auto result = nvdrv->Close(fd);

    IPC::RequestBuilder rb{ctx, 2};
    rb.Push(result);
}

void NVDRV::Initialize(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::RequestBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0);
}

void NVDRV::SetClientPID(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    u64 pid = rp.Pop<u64>();
    u64 unk = rp.Pop<u64>();

    LOG_WARNING(Service, "(STUBBED) called, pid=0x%llx, unk=0x%llx", pid, unk);

    IPC::RequestBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

NVDRV::NVDRV(std::shared_ptr<Module> nvdrv, const char* name)
    : ServiceFramework(name), nvdrv(std::move(nvdrv)) {
    static const FunctionInfo functions[] = {
        {0, &NVDRV::Open, "Open"},
        {1, &NVDRV::Ioctl, "Ioctl"},
        {2, &NVDRV::Close, "Close"},
        {3, &NVDRV::Initialize, "Initialize"},
        {8, &NVDRV::SetClientPID, "SetClientPID"},
    };
    RegisterHandlers(functions);
}

} // namespace Nvidia
} // namespace Service
