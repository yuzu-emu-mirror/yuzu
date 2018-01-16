// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/pctl/pctl_a.h"

namespace Service {
namespace PCTL {

class IParentalControlService final : public ServiceFramework<IParentalControlService> {
public:
    IParentalControlService() : ServiceFramework("IParentalControlService") {}
};

void PCTL_A::GetService(Kernel::HLERequestContext& ctx) {
    IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IParentalControlService>();
    LOG_DEBUG(Service, "called");
}

PCTL_A::PCTL_A() : ServiceFramework("pctl:a") {
    static const FunctionInfo functions[] = {
        {0, &PCTL_A::GetService, "GetService"},
    };
    RegisterHandlers(functions);
}

} // namespace PCTL
} // namespace Service
