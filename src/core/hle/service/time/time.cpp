// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <chrono>
#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/service/time/time.h"

namespace Service {
namespace Time {

class ISystemClock final : public ServiceFramework<ISystemClock> {
public:
    ISystemClock() : ServiceFramework("ISystemClock") {
        static const FunctionInfo functions[] = {
            {0, &ISystemClock::GetCurrentTime, "GetCurrentTime"},
        };
        RegisterHandlers(functions);
    }

private:
    void GetCurrentTime(Kernel::HLERequestContext& ctx) {
        const s64 time_since_epoch{std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count()};
        IPC::RequestBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u64>(time_since_epoch);
        LOG_DEBUG(Service, "called");
    }
};

class ISteadyClock final : public ServiceFramework<ISteadyClock> {
public:
    ISteadyClock() : ServiceFramework("ISteadyClock") {}
};

class ITimeZoneService final : public ServiceFramework<ITimeZoneService> {
public:
    ITimeZoneService() : ServiceFramework("ITimeZoneService") {
        static const FunctionInfo functions[] = {
            {0, &ITimeZoneService::GetDeviceLocationName, "GetDeviceLocationName"},
            {101, &ITimeZoneService::ToCalendarTimeWithMyRule, "ToCalendarTimeWithMyRule"},
        };
        RegisterHandlers(functions);
    }

private:
    void GetDeviceLocationName(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service, "(STUBBED) called");
        LocationName name{};
        IPC::RequestBuilder rb{ctx, 11};
        rb.Push(RESULT_SUCCESS);
        rb.PushRaw(name);
    }

    void ToCalendarTimeWithMyRule(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        u64 posixTime = rp.Pop<u64>();

        LOG_WARNING(Service, "(STUBBED) called, posixTime=0x%016llX", posixTime);

        CalendarTime calendarTime{2018, 1, 1, 0, 0, 0};
        CalendarAdditionalInfo additionalInfo{};
        IPC::RequestBuilder rb{ctx, 10};
        rb.Push(RESULT_SUCCESS);
        rb.PushRaw(calendarTime);
        rb.PushRaw(additionalInfo);
    }
};

void TIME::GetStandardUserSystemClock(Kernel::HLERequestContext& ctx) {
    auto client_port = std::make_shared<ISystemClock>()->CreatePort();
    auto session = client_port->Connect();
    if (session.Succeeded()) {
        LOG_DEBUG(Service, "called, initialized ISystemClock -> session=%u",
                  (*session)->GetObjectId());
        IPC::RequestBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushMoveObjects(std::move(session).Unwrap());
    } else {
        UNIMPLEMENTED();
    }
}

void TIME::GetStandardNetworkSystemClock(Kernel::HLERequestContext& ctx) {
    auto client_port = std::make_shared<ISystemClock>()->CreatePort();
    auto session = client_port->Connect();
    if (session.Succeeded()) {
        LOG_DEBUG(Service, "called, initialized ISystemClock -> session=%u",
                  (*session)->GetObjectId());
        IPC::RequestBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushMoveObjects(std::move(session).Unwrap());
    } else {
        UNIMPLEMENTED();
    }
}

void TIME::GetStandardSteadyClock(Kernel::HLERequestContext& ctx) {
    auto client_port = std::make_shared<ISteadyClock>()->CreatePort();
    auto session = client_port->Connect();
    if (session.Succeeded()) {
        LOG_DEBUG(Service, "called, initialized ISteadyClock -> session=%u",
                  (*session)->GetObjectId());
        IPC::RequestBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushMoveObjects(std::move(session).Unwrap());
    } else {
        UNIMPLEMENTED();
    }
}

void TIME::GetTimeZoneService(Kernel::HLERequestContext& ctx) {
    IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<ITimeZoneService>();
    LOG_DEBUG(Service, "called");
}

TIME::TIME(const char* name) : ServiceFramework(name) {
    static const FunctionInfo functions[] = {
        {0x00000000, &TIME::GetStandardUserSystemClock, "GetStandardUserSystemClock"},
        {0x00000001, &TIME::GetStandardNetworkSystemClock, "GetStandardNetworkSystemClock"},
        {0x00000002, &TIME::GetStandardSteadyClock, "GetStandardSteadyClock"},
        {0x00000003, &TIME::GetTimeZoneService, "GetTimeZoneService"},
    };
    RegisterHandlers(functions);
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<TIME>("time:a")->InstallAsService(service_manager);
    std::make_shared<TIME>("time:r")->InstallAsService(service_manager);
    std::make_shared<TIME>("time:s")->InstallAsService(service_manager);
    std::make_shared<TIME>("time:u")->InstallAsService(service_manager);
}

} // namespace Time
} // namespace Service
