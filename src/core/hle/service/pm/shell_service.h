// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/cmif_types.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/service.h"

namespace Service::PM {

class ShellService final : public ServiceFramework<ShellService> {
public:
    explicit ShellService(Core::System& system_);

private:
    Result GetApplicationProcessIdForShell(Out<ProcessId> out_process_id);
};

} // namespace Service::PM
