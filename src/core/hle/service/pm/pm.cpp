// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/hle/service/pm/boot_mode_service.h"
#include "core/hle/service/pm/debug_monitor_service.h"
#include "core/hle/service/pm/information_service.h"
#include "core/hle/service/pm/pm.h"
#include "core/hle/service/pm/shell_service.h"
#include "core/hle/service/server_manager.h"

namespace Service::PM {

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("pm:bm", std::make_shared<BootModeService>(system));
    server_manager->RegisterNamedService("pm:dmnt", std::make_shared<DebugMonitorService>(system));
    server_manager->RegisterNamedService("pm:info", std::make_shared<InformationService>(system));
    server_manager->RegisterNamedService("pm:shell", std::make_shared<ShellService>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::PM
