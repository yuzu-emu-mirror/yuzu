// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/time/time.h"

namespace Service::Time {

class TIME final : public Module::Interface {
public:
    explicit TIME(std::shared_ptr<Module> time, const char* name);
};

} // namespace Service::Time
