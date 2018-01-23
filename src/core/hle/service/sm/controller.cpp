// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/sm/controller.h"

namespace Service {
namespace SM {

void Controller::ConvertSessionToDomain(Kernel::HLERequestContext& ctx) {
    auto domain = Kernel::Domain::CreateFromSession(*ctx.ServerSession()->parent).Unwrap();

    IPC::RequestBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push(static_cast<u32>(domain->request_handlers.size()));

    LOG_DEBUG(Service, "called, domain=%d", domain->GetObjectId());
}

void Controller::DuplicateSession(Kernel::HLERequestContext& ctx) {
    IPC::RequestBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    // TODO(Subv): Check if this is correct
    if (ctx.IsDomain())
        rb.PushMoveObjects(ctx.Domain());
    else
        rb.PushMoveObjects(ctx.ServerSession());

    LOG_DEBUG(Service, "called");
}

void Controller::DuplicateSessionEx(Kernel::HLERequestContext& ctx) {
    DuplicateSession(ctx);

    LOG_WARNING(Service, "(STUBBED) called, using DuplicateSession");
}

void Controller::QueryPointerBufferSize(Kernel::HLERequestContext& ctx) {
    IPC::RequestBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0x500);

    LOG_WARNING(Service, "(STUBBED) called");
}

Controller::Controller() : ServiceFramework("IpcController") {
    static const FunctionInfo functions[] = {
        {0x00000000, &Controller::ConvertSessionToDomain, "ConvertSessionToDomain"},
        {0x00000001, nullptr, "ConvertDomainToSession"},
        {0x00000002, &Controller::DuplicateSession, "DuplicateSession"},
        {0x00000003, &Controller::QueryPointerBufferSize, "QueryPointerBufferSize"},
        {0x00000004, &Controller::DuplicateSessionEx, "DuplicateSessionEx"},
    };
    RegisterHandlers(functions);
}

} // namespace SM
} // namespace Service
