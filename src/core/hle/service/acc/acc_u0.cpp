// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/acc/acc_u0.h"

namespace Service {
namespace Account {

void ACC_U0::InitializeApplicationInfo(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::RequestBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

ACC_U0::ACC_U0() : ServiceFramework("acc:u0") {
    static const FunctionInfo functions[] = {
        {100, &ACC_U0::InitializeApplicationInfo, "InitializeApplicationInfo"},
    };
    RegisterHandlers(functions);
}

} // namespace Account
} // namespace Service
