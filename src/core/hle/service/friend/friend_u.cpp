// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/friend/friend_u.h"

namespace Service::Friend {

Friend_U::Friend_U(std::shared_ptr<Module> module)
    : Module::Interface(std::move(module), "friend:u") {
    static const FunctionInfo functions[] = {
        {0, &Friend_U::CreateFriendService, "CreateFriendService"},
        {1, nullptr, "CreateNotificationService"},
    };
    RegisterHandlers(functions);
}

} // namespace Service::Friend
