// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nifm/nifm.h"
#include "core/hle/service/nifm/nifm_s.h"

namespace Service {
namespace NIFM {

void NIFM_S::CreateGeneralServiceOld(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IGeneralService>();
    LOG_DEBUG(Service, "called");
}

void NIFM_S::CreateGeneralService(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IGeneralService>();
    LOG_DEBUG(Service, "called");
}

NIFM_S::NIFM_S() : ServiceFramework("nifm:s") {
    static const FunctionInfo functions[] = {
        {4, &NIFM_S::CreateGeneralServiceOld, "CreateGeneralServiceOld"},
        {5, &NIFM_S::CreateGeneralService, "CreateGeneralService"},
    };
    RegisterHandlers(functions);
}

} // namespace NIFM
} // namespace Service
