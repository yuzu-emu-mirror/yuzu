// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/sm/controller.h"

namespace Service {
namespace SM {

void Controller::ConvertSessionToDomain(Kernel::HLERequestContext& ctx) {
    ASSERT_MSG(!ctx.Session()->IsDomain(), "session is alread a domain");
    ctx.Session()->ConvertToDomain();

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(1); // Converted sessions start with 1 request handler

    LOG_DEBUG(Service, "called, server_session=%d", ctx.Session()->GetObjectId());
}

void Controller::DuplicateSession(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1, true};
    rb.Push(RESULT_SUCCESS);
    rb.PushMoveObjects(ctx.Session());

    LOG_DEBUG(Service, "called");
}

void Controller::DuplicateSessionEx(Kernel::HLERequestContext& ctx) {
    DuplicateSession(ctx);

    LOG_WARNING(Service, "(STUBBED) called, using DuplicateSession");
}

void Controller::QueryPointerBufferSize(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 3};
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
