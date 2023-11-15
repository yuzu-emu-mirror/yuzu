// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "service/audio/audctl.h"
#include "service/audio/audin_u.h"
#include "service/audio/audio.h"
#include "service/audio/audout_u.h"
#include "service/audio/audrec_a.h"
#include "service/audio/audrec_u.h"
#include "service/audio/audren_u.h"
#include "service/audio/hwopus.h"
#include "service/server_manager.h"
#include "service/service.h"

namespace Service::Audio {

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("audctl", std::make_shared<AudCtl>(system));
    server_manager->RegisterNamedService("audout:u", std::make_shared<AudOutU>(system));
    server_manager->RegisterNamedService("audin:u", std::make_shared<AudInU>(system));
    server_manager->RegisterNamedService("audrec:a", std::make_shared<AudRecA>(system));
    server_manager->RegisterNamedService("audrec:u", std::make_shared<AudRecU>(system));
    server_manager->RegisterNamedService("audren:u", std::make_shared<AudRenU>(system));
    server_manager->RegisterNamedService("hwopus", std::make_shared<HwOpus>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::Audio
