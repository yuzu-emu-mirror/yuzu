// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/time/time.h"

namespace Service {
namespace Time {

class TIME_S final : public Module::Interface {
public:
    explicit TIME_S(std::shared_ptr<Module> time);
};

} // namespace Time
} // namespace Service
