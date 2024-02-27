// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/pm/pm_types.h"
#include "core/hle/service/service.h"

namespace Service::PM {

class BootModeService final : public ServiceFramework<BootModeService> {
public:
    explicit BootModeService(Core::System& system_);

private:
    void GetBootMode(HLERequestContext& ctx);
    void SetMaintenanceBoot(HLERequestContext& ctx);

    BootMode boot_mode = BootMode::Normal;
};

} // namespace Service::PM
