// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/pm/boot_mode_service.h"

namespace Service::PM {

BootModeService::BootModeService(Core::System& system_) : ServiceFramework{system_, "pm:bm"} {
    static const FunctionInfo functions[] = {
        {0, &BootModeService::GetBootMode, "GetBootMode"},
        {1, &BootModeService::SetMaintenanceBoot, "SetMaintenanceBoot"},
    };
    RegisterHandlers(functions);
}

void BootModeService::GetBootMode(HLERequestContext& ctx) {
    LOG_DEBUG(Service_PM, "called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(boot_mode);
}

void BootModeService::SetMaintenanceBoot(HLERequestContext& ctx) {
    LOG_DEBUG(Service_PM, "called");

    boot_mode = BootMode::Maintenance;

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

} // namespace Service::PM
