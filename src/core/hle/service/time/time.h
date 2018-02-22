// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace Time {

// TODO(Rozelette) RE this structure
struct LocationName {
    INSERT_PADDING_BYTES(0x24);
};
static_assert(sizeof(LocationName) == 0x24, "LocationName is incorrect size");

struct CalendarTime {
    u16_le year;
    u8 month; // Starts at 1
    u8 day;   // Starts at 1
    u8 hour;
    u8 minute;
    u8 second;
    INSERT_PADDING_BYTES(1);
};
static_assert(sizeof(CalendarTime) == 0x8, "CalendarTime structure has incorrect size");

// TODO(Rozelette) RE this structure
struct CalendarAdditionalInfo {
    INSERT_PADDING_BYTES(0x18);
};
static_assert(sizeof(CalendarAdditionalInfo) == 0x18,
              "CalendarAdditionalInfo structure has incorrect size");

// TODO(bunnei) RE this structure
struct SystemClockContext {
    INSERT_PADDING_BYTES(0x20);
};
static_assert(sizeof(SystemClockContext) == 0x20,
              "SystemClockContext structure has incorrect size");

struct SteadyClockTimePoint {
    u64 value;
    INSERT_PADDING_WORDS(4);
};
static_assert(sizeof(SteadyClockTimePoint) == 0x18, "SteadyClockTimePoint is incorrect size");

class Module final {
public:
    class Interface : public ServiceFramework<Interface> {
    public:
        Interface(std::shared_ptr<Module> time, const char* name);

        void GetStandardUserSystemClock(Kernel::HLERequestContext& ctx);
        void GetStandardNetworkSystemClock(Kernel::HLERequestContext& ctx);
        void GetStandardSteadyClock(Kernel::HLERequestContext& ctx);
        void GetTimeZoneService(Kernel::HLERequestContext& ctx);
        void GetStandardLocalSystemClock(Kernel::HLERequestContext& ctx);

    protected:
        std::shared_ptr<Module> time;
    };
};

/// Registers all Time services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace Time
} // namespace Service
