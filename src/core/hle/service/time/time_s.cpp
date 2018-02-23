// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/time/time_s.h"

namespace Service {
namespace Time {

TIME_S::TIME_S(std::shared_ptr<Module> time) : Module::Interface(std::move(time), "time:s") {
    static const FunctionInfo functions[] = {
        {0, &TIME_S::GetStandardUserSystemClock, "GetStandardUserSystemClock"},
        {1, &TIME_S::GetStandardNetworkSystemClock, "GetStandardNetworkSystemClock"},
        {2, &TIME_S::GetStandardSteadyClock, "GetStandardSteadyClock"},
        {3, &TIME_S::GetTimeZoneService, "GetTimeZoneService"},
        {4, &TIME_S::GetStandardLocalSystemClock, "GetStandardLocalSystemClock"},
    };
    RegisterHandlers(functions);
}

} // namespace Time
} // namespace Service
