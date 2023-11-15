// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "service/server_manager.h"
#include "service/set/set.h"
#include "service/set/set_cal.h"
#include "service/set/set_fd.h"
#include "service/set/set_sys.h"
#include "service/set/settings.h"

namespace Service::Set {

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("set", std::make_shared<SET>(system));
    server_manager->RegisterNamedService("set:cal", std::make_shared<SET_CAL>(system));
    server_manager->RegisterNamedService("set:fd", std::make_shared<SET_FD>(system));
    server_manager->RegisterNamedService("set:sys", std::make_shared<SET_SYS>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::Set
