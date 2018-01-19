// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/time/time_s.h"

namespace Service {
namespace Time {

TIME_S::TIME_S(std::shared_ptr<Module> time) : Module::Interface(std::move(time), "time:s") {
    static const FunctionInfo functions[] = {
        {0, &TIME_S::GetStandardUserSystemClock, "GetStandardUserSystemClock"},
    };
    RegisterHandlers(functions);
}

} // namespace Time
} // namespace Service
