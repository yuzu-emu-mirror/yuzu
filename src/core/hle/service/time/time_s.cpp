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
