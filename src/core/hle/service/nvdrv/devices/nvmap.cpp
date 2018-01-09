// Copyright 2018 Yuzu Emulator Team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/nvdrv/devices/nvmap.h"

namespace Service {
namespace NVDRV {
namespace Devices {

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

} // namespace Devices
} // namespace NVDRV
} // namespace Service
