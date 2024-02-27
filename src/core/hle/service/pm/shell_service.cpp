// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/pm/pm_types.h"
#include "core/hle/service/pm/shell_service.h"

namespace Service::PM {

ShellService::ShellService(Core::System& system_) : ServiceFramework{system_, "pm:shell"} {
    // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "LaunchProgram"},
            {1, nullptr, "TerminateProcess"},
            {2, nullptr, "TerminateProgram"},
            {3, nullptr, "GetProcessEventHandle"},
            {4, nullptr, "GetProcessEventInfo"},
            {5, nullptr, "NotifyBootFinished"},
            {6, &ShellService::GetApplicationProcessIdForShell, "GetApplicationProcessIdForShell"},
            {7, nullptr, "BoostSystemMemoryResourceLimit"},
            {8, nullptr, "BoostApplicationThreadResourceLimit"},
            {9, nullptr, "GetBootFinishedEventHandle"},
        };
    // clang-format on

    RegisterHandlers(functions);
}

void ShellService::GetApplicationProcessIdForShell(HLERequestContext& ctx) {
    LOG_DEBUG(Service_PM, "called");
    auto list = kernel.GetProcessList();
    GetApplicationPidGeneric(ctx, list);
}

} // namespace Service::PM
