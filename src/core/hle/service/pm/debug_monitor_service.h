// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/service.h"

namespace Service::PM {

class DebugMonitorService final : public ServiceFramework<DebugMonitorService> {
public:
    explicit DebugMonitorService(Core::System& system_);

private:
    void GetProcessId(HLERequestContext& ctx);
    void GetApplicationProcessId(HLERequestContext& ctx);
    void AtmosphereGetProcessInfo(HLERequestContext& ctx);
};
} // namespace Service::PM
