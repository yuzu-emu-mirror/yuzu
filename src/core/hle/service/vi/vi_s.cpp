// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/vi/vi.h"
#include "core/hle/service/vi/vi_s.h"

namespace Service {
namespace VI {

void VI_S::GetDisplayService(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IApplicationDisplayService>(nv_flinger);
}

VI_S::VI_S(std::shared_ptr<NVFlinger::NVFlinger> nv_flinger)
    : ServiceFramework("vi:s"), nv_flinger(std::move(nv_flinger)) {
    static const FunctionInfo functions[] = {
        {1, &VI_S::GetDisplayService, "GetDisplayService"},
        {3, nullptr, "GetDisplayServiceWithProxyNameExchange"},
    };
    RegisterHandlers(functions);
}

} // namespace VI
} // namespace Service
