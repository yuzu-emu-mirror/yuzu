// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/domain.h"
#include "core/hle/service/sm/controller.h"

namespace Service {
namespace SM {

void Controller::ConvertSessionToDomain(Kernel::HLERequestContext& ctx) {
    auto domain = Kernel::Domain::CreateFromSession(*ctx.ServerSession()->parent).Unwrap();

    IPC::RequestBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Skip(1, true);
    rb.Push<u32>(static_cast<u32>(domain->request_handlers.size()));

    LOG_DEBUG(Service, "called, domain=%d", domain->GetObjectId());
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

} // namespace SM
} // namespace Service
