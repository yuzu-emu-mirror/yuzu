// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/pctl/pctl_a.h"

namespace Service {
namespace PCTL {

void PCTL_A::GetService(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::RequestBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
    // TODO(Subv): This should return an IParentalControlService interface.
}

PCTL_A::PCTL_A() : ServiceFramework("pctl:a") {
    static const FunctionInfo functions[] = {
        {0, &PCTL_A::GetService, "GetService"},
    };
    RegisterHandlers(functions);
}

} // namespace PCTL
} // namespace Service
