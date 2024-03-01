// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/cmif_serialization.h"
#include "core/hle/service/pm/boot_mode_service.h"

namespace Service::PM {

BootModeService::BootModeService(Core::System& system_) : ServiceFramework{system_, "pm:bm"} {
    static const FunctionInfo functions[] = {
        {0, C<&BootModeService::GetBootMode>, "GetBootMode"},
        {1, C<&BootModeService::SetMaintenanceBoot>, "SetMaintenanceBoot"},
    };
    RegisterHandlers(functions);
}

Result BootModeService::GetBootMode(Out<u32> out_boot_mode) {
    LOG_DEBUG(Service_PM, "called");

    *out_boot_mode = static_cast<u32>(boot_mode);

    R_SUCCEED();
}

Result BootModeService::SetMaintenanceBoot() {
    LOG_DEBUG(Service_PM, "called");

    boot_mode = BootMode::Maintenance;

    R_SUCCEED();
}

} // namespace Service::PM
