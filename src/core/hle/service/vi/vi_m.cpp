// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/vi/vi.h"
#include "core/hle/service/vi/vi_m.h"

namespace Service {
namespace VI {

void VI_M::GetDisplayService(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
    rb.PushIpcInterface<IApplicationDisplayService>();
}

VI_M::VI_M() : ServiceFramework("vi:m") {
    static const FunctionInfo functions[] = {
        {2, &VI_M::GetDisplayService, "GetDisplayService"},
        {3, nullptr, "GetDisplayServiceWithProxyNameExchange"},
    };
    RegisterHandlers(functions);
}

} // namespace VI
} // namespace Service
