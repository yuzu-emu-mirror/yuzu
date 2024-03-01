// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/core.h"
#include "core/hle/service/dmnt/cheat_interface.h"
#include "core/hle/service/dmnt/cheat_process_manager.h"
#include "core/hle/service/dmnt/cheat_virtual_machine.h"
#include "core/hle/service/dmnt/dmnt.h"
#include "core/hle/service/server_manager.h"

namespace Service::DMNT {

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    auto& cheat_manager = system.GetCheatManager();
    auto cheat_vm = std::make_unique<CheatVirtualMachine>(cheat_manager);
    cheat_manager.SetVirtualMachine(std::move(cheat_vm));

    server_manager->RegisterNamedService("dmnt:cht",
                                         std::make_shared<ICheatInterface>(system, cheat_manager));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::DMNT
