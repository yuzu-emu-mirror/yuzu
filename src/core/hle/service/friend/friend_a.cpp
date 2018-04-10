// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/friend/friend_a.h"

namespace Service {
namespace Friend {

Friend_A::Friend_A(std::shared_ptr<Module> module)
    : Module::Interface(std::move(module), "friend:a") {
    static const FunctionInfo functions[] = {
        {0, &Friend_A::CreateFriendService, "CreateFriendService"},
        {1, nullptr, "CreateNotificationService"},
    };
    RegisterHandlers(functions);
}

} // namespace Friend
} // namespace Service
