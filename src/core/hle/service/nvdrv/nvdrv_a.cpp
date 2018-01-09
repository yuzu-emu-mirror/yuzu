// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/nvdrv/nvdrv_a.h"

namespace Service {
namespace NVDRV {

class nvhost_as_gpu : public nvdevice {
public:
    u32 ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) override {
        ASSERT(false, "Unimplemented");
        return 0;
    }
};

VAddr nvmap::GetObjectAddress(u32 handle) const {
    auto itr = handles.find(handle);
    ASSERT(itr != handles.end());

    auto object = itr->second;
    ASSERT(object->status == Object::Status::Allocated);
    return object->addr;
}

u32 nvmap::ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) {
    switch (command) {
    case IocCreateCommand:
        return IocCreate(input, output);
    case IocAllocCommand:
        return IocAlloc(input, output);
    case IocGetIdCommand:
        return IocGetId(input, output);
    case IocFromIdCommand:
        return IocFromId(input, output);
    case IocParamCommand:
        return IocParam(input, output);
    }

    ASSERT(false, "Unimplemented");
}

u32 nvmap::IocCreate(const std::vector<u8>& input, std::vector<u8>& output) {
    IocCreateParams params;
    std::memcpy(&params, input.data(), sizeof(params));

    // Create a new nvmap object and obtain a handle to it.
    auto object = std::make_shared<Object>();
    object->id = next_id++;
    object->size = params.size;
    object->status = Object::Status::Created;

    u32 handle = next_handle++;
    handles[handle] = std::move(object);

    LOG_WARNING(Service, "(STUBBED) size 0x%08X", params.size);

    params.handle = handle;

    std::memcpy(output.data(), &params, sizeof(params));
    return 0;
}

u32 nvmap::IocAlloc(const std::vector<u8>& input, std::vector<u8>& output) {
    IocAllocParams params;
    std::memcpy(&params, input.data(), sizeof(params));

    auto itr = handles.find(params.handle);
    ASSERT(itr != handles.end());

    auto object = itr->second;
    object->flags = params.flags;
    object->align = params.align;
    object->kind = params.kind;
    object->addr = params.addr;
    object->status = Object::Status::Allocated;

    LOG_WARNING(Service, "(STUBBED) Allocated address 0x%llx", params.addr);

    std::memcpy(output.data(), &params, sizeof(params));
    return 0;
}

u32 nvmap::IocGetId(const std::vector<u8>& input, std::vector<u8>& output) {
    IocGetIdParams params;
    std::memcpy(&params, input.data(), sizeof(params));

    LOG_WARNING(Service, "called");

    auto itr = handles.find(params.handle);
    ASSERT(itr != handles.end());

    params.id = itr->second->id;

    std::memcpy(output.data(), &params, sizeof(params));
    return 0;
}

u32 nvmap::IocFromId(const std::vector<u8>& input, std::vector<u8>& output) {
    IocFromIdParams params;
    std::memcpy(&params, input.data(), sizeof(params));

    LOG_WARNING(Service, "(STUBBED) called");

    auto itr = std::find_if(handles.begin(), handles.end(),
                            [&](const auto& entry) { return entry.second->id == params.id; });
    ASSERT(itr != handles.end());

    // Make a new handle for the object
    u32 handle = next_handle++;
    handles[handle] = itr->second;

    params.handle = handle;

    std::memcpy(output.data(), &params, sizeof(params));
    return 0;
}

u32 nvmap::IocParam(const std::vector<u8>& input, std::vector<u8>& output) {
    enum class ParamTypes { Size = 1, Alignment = 2, Base = 3, Heap = 4, Kind = 5, Compr = 6 };

    IocParamParams params;
    std::memcpy(&params, input.data(), sizeof(params));

    LOG_WARNING(Service, "(STUBBED) called type=%u", params.type);

    auto itr = handles.find(params.handle);
    ASSERT(itr != handles.end());

    auto object = itr->second;
    ASSERT(object->status == Object::Status::Allocated);

    switch (static_cast<ParamTypes>(params.type)) {
    case ParamTypes::Size:
        params.value = object->size;
        break;
    case ParamTypes::Alignment:
        params.value = object->align;
        break;
    case ParamTypes::Heap:
        // TODO(Subv): Seems to be a hardcoded value?
        params.value = 0x40000000;
        break;
    case ParamTypes::Kind:
        params.value = object->kind;
        break;
    default:
        ASSERT(false, "Unimplemented");
    }

    std::memcpy(output.data(), &params, sizeof(params));
    return 0;
}

u32 nvdisp_disp0::ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) {
    ASSERT(false, "Unimplemented");
    return 0;
}

void nvdisp_disp0::flip(u32 buffer_handle, u32 offset, u32 format, u32 width, u32 height,
                        u32 stride) {
    VAddr addr = nvmap_dev->GetObjectAddress(buffer_handle);
    LOG_WARNING(Service,
                "Drawing from address %llx offset %08X Width %u Height %u Stride %u Format %u",
                addr, offset, width, height, stride, format);
}

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

    auto nvmap_dev = std::make_shared<nvmap>();
    devices["/dev/nvhost-as-gpu"] = std::make_shared<nvhost_as_gpu>();
    devices["/dev/nvmap"] = nvmap_dev;
    devices["/dev/nvdisp_disp0"] = std::make_shared<nvdisp_disp0>(nvmap_dev);
}

} // namespace NVDRV
} // namespace Service
