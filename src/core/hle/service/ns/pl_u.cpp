// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/ipc_helpers.h"
#include "core/hle/service/ns/pl_u.h"

namespace Service {
namespace NS {

static constexpr u64 SHARED_FONT_SIZE{0x2000}; // TODO(bunnei): Find the correct value for this

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<PL_U>()->InstallAsService(service_manager);
}

PL_U::PL_U() : ServiceFramework("pl:u") {
    static const FunctionInfo functions[] = {
        {1, &PL_U::GetLoadState, "GetLoadState"},
        {2, &PL_U::GetSize, "GetSize"},
        {3, &PL_U::GetSharedMemoryAddressOffset, "GetSharedMemoryAddressOffset"},
        {4, &PL_U::GetSharedMemoryNativeHandle, "GetSharedMemoryNativeHandle"}};
    RegisterHandlers(functions);

    shared_mem = Kernel::SharedMemory::Create(
        Kernel::g_current_process, SHARED_FONT_SIZE, Kernel::MemoryPermission::ReadWrite,
        Kernel::MemoryPermission::Read, 0, Kernel::MemoryRegion::BASE, "PL_U:SharedMemory");
}

void PL_U::GetLoadState(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(1); // 0 = Loading, 1 = Loaded
}

void PL_U::GetSize(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(SHARED_FONT_SIZE);
}

void PL_U::GetSharedMemoryAddressOffset(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0x1000);
}

void PL_U::GetSharedMemoryNativeHandle(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service, "called");
    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushCopyObjects(shared_mem);
}

} // namespace NS
} // namespace Service
