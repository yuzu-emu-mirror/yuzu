// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/time/time_u.h"

namespace Service {
namespace Time {

TIME_U::TIME_U(std::shared_ptr<Module> time) : Module::Interface(std::move(time), "time:u") {
    static const FunctionInfo functions[] = {
        {0, &TIME_U::GetStandardUserSystemClock, "GetStandardUserSystemClock"},
        {1, &TIME_U::GetStandardNetworkSystemClock, "GetStandardNetworkSystemClock"},
        {2, &TIME_U::GetStandardSteadyClock, "GetStandardSteadyClock"},
        {3, &TIME_U::GetTimeZoneService, "GetTimeZoneService"},
        {4, &TIME_U::GetStandardLocalSystemClock, "GetStandardLocalSystemClock"},
        {5, nullptr, "GetEphemeralNetworkSystemClock"},
        {50, nullptr, "SetStandardSteadyClockInternalOffset"},
        {100, nullptr, "IsStandardUserSystemClockAutomaticCorrectionEnabled"},
        {101, nullptr, "SetStandardUserSystemClockAutomaticCorrectionEnabled"},
        {102, nullptr, "GetStandardUserSystemClockInitialYear"},
        {200, nullptr, "IsStandardNetworkSystemClockAccuracySufficient"},
        {300, nullptr, "CalculateMonotonicSystemClockBaseTimePoint"},
        {400, nullptr, "GetClockSnapshot"},
        {401, nullptr, "GetClockSnapshotFromSystemClockContext"},
        {500, nullptr, "CalculateStandardUserSystemClockDifferenceByUser"},
        {501, nullptr, "CalculateSpanBetween"},
    };
    RegisterHandlers(functions);
}

} // namespace Time
} // namespace Service
