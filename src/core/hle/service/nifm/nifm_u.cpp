// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nifm/nifm.h"
#include "core/hle/service/nifm/nifm_u.h"

namespace Service {
namespace NIFM {

void NIFM_U::CreateGeneralServiceOld(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IGeneralService>();
    LOG_DEBUG(Service_NIFM, "called");
}

void NIFM_U::CreateGeneralService(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IGeneralService>();
    LOG_DEBUG(Service_NIFM, "called");
}

NIFM_U::NIFM_U() : ServiceFramework("nifm:u") {
    static const FunctionInfo functions[] = {
        {4, &NIFM_U::CreateGeneralServiceOld, "CreateGeneralServiceOld"},
        {5, &NIFM_U::CreateGeneralService, "CreateGeneralService"},
    };
    RegisterHandlers(functions);
}

} // namespace NIFM
} // namespace Service
