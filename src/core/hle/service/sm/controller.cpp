// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/sm/controller.h"

namespace Service {
namespace SM {

/**
 * Controller::QueryPointerBufferSize service function
 *  Inputs:
 *      0: 0x00000003
 *  Outputs:
 *      1: ResultCode
 *      3: Size of memory
 */
void Controller::QueryPointerBufferSize(Kernel::HLERequestContext& ctx) {
    IPC::RequestBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
    rb.Push(0x0U);
    rb.Push(0x500U);
    LOG_WARNING(Service, "(STUBBED) called");
}

Controller::Controller() : ServiceFramework("IpcController") {
    static const FunctionInfo functions[] = {
        {0x00000000, nullptr, "ConvertSessionToDomain"},
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
