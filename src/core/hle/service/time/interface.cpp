// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/time/interface.h"

namespace Service::Time {

TIME::TIME(std::shared_ptr<Module> time, const char* name)
    : Module::Interface(std::move(time), name) {
    static const FunctionInfo functions[] = {
        {0, &TIME::GetStandardUserSystemClock, "GetStandardUserSystemClock"},
        {1, &TIME::GetStandardNetworkSystemClock, "GetStandardNetworkSystemClock"},
        {2, &TIME::GetStandardSteadyClock, "GetStandardSteadyClock"},
        {3, &TIME::GetTimeZoneService, "GetTimeZoneService"},
        {4, &TIME::GetStandardLocalSystemClock, "GetStandardLocalSystemClock"},
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

} // namespace Service::Time
