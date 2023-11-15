// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "service/server_manager.h"
#include "service/sockets/bsd.h"
#include "service/sockets/nsd.h"
#include "service/sockets/sfdnsres.h"
#include "service/sockets/sockets.h"

namespace Service::Sockets {

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("bsd:s", std::make_shared<BSD>(system, "bsd:s"));
    server_manager->RegisterNamedService("bsd:u", std::make_shared<BSD>(system, "bsd:u"));
    server_manager->RegisterNamedService("bsdcfg", std::make_shared<BSDCFG>(system));
    server_manager->RegisterNamedService("nsd:a", std::make_shared<NSD>(system, "nsd:a"));
    server_manager->RegisterNamedService("nsd:u", std::make_shared<NSD>(system, "nsd:u"));
    server_manager->RegisterNamedService("sfdnsres", std::make_shared<SFDNSRES>(system));
    server_manager->StartAdditionalHostThreads("bsdsocket", 2);
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::Sockets
