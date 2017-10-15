// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/sm/controller.h"

namespace Service {
namespace SM {

/**
 * Controller::ConvertSessionToDomain service function
 *  Inputs:
 *      0: 0x00000000
 *  Outputs:
 *      0: ResultCode
 *      2: Handle of domain
 */
void Controller::ConvertSessionToDomain(Kernel::HLERequestContext& ctx) {
    ctx.Session()->ConvertToDomain();
    IPC::RequestBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Skip(1, true);
    Kernel::Handle handle = Kernel::g_handle_table.Create(ctx.Session()).Unwrap();
    rb.Push(handle);
    LOG_DEBUG(Service, "called, handle=0x%08x", handle);
}

/**
 * Controller::QueryPointerBufferSize service function
 *  Inputs:
 *      0: 0x00000003
 *  Outputs:
 *      0: ResultCode
 *      2: Size of memory
 */
void Controller::QueryPointerBufferSize(Kernel::HLERequestContext& ctx) {
    IPC::RequestBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Skip(1, true);
    rb.Push<u32>(0x500);
    LOG_WARNING(Service, "(STUBBED) called");
}

Controller::Controller() : ServiceFramework("IpcController") {
    static const FunctionInfo functions[] = {
        {0x00000000, &Controller::ConvertSessionToDomain, "ConvertSessionToDomain"},
        {0x00000001, nullptr, "ConvertDomainToSession"},
        {0x00000002, nullptr, "DuplicateSession"},
        {0x00000003, &Controller::QueryPointerBufferSize, "QueryPointerBufferSize"},
        {0x00000004, nullptr, "DuplicateSessionEx"},
    };
    RegisterHandlers(functions);
}

Controller::~Controller() = default;

} // namespace SM
} // namespace Service
