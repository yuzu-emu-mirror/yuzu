// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/service.h"

namespace Service::PM {

class InformationService final : public ServiceFramework<InformationService> {
public:
    explicit InformationService(Core::System& system_);

private:
    void GetProgramId(HLERequestContext& ctx);
    void AtmosphereGetProcessId(HLERequestContext& ctx);
};

} // namespace Service::PM
