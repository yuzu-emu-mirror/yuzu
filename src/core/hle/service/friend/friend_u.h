// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/friend/friend.h"

namespace Service::Friend {

class Friend_U final : public Module::Interface {
public:
    explicit Friend_U(std::shared_ptr<Module> module);
};

} // namespace Service::Friend
