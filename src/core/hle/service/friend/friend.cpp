// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/friend/friend.h"
#include "core/hle/service/friend/friend_a.h"
#include "core/hle/service/friend/friend_u.h"

namespace Service::Friend {

void Module::Interface::CreateFriendService(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
    NGLOG_WARNING(Service_Friend, "(STUBBED) called");
}

Module::Interface::Interface(std::shared_ptr<Module> module, const char* name)
    : ServiceFramework(name), module(std::move(module)) {}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    auto module = std::make_shared<Module>();
    std::make_shared<Friend_A>(module)->InstallAsService(service_manager);
    std::make_shared<Friend_U>(module)->InstallAsService(service_manager);
}

} // namespace Service::Friend
