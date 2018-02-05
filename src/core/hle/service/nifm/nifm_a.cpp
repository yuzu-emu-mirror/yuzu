// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nifm/nifm.h"
#include "core/hle/service/nifm/nifm_a.h"

namespace Service {
namespace NIFM {

void NIFM_A::CreateGeneralServiceOld(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IGeneralService>();
    LOG_DEBUG(Service_NIFM, "called");
}

void NIFM_A::CreateGeneralService(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IGeneralService>();
    LOG_DEBUG(Service_NIFM, "called");
}

NIFM_A::NIFM_A() : ServiceFramework("nifm:a") {
    static const FunctionInfo functions[] = {
        {4, &NIFM_A::CreateGeneralServiceOld, "CreateGeneralServiceOld"},
        {5, &NIFM_A::CreateGeneralService, "CreateGeneralService"},
    };
    RegisterHandlers(functions);
}

} // namespace NIFM
} // namespace Service
