// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <chrono>
#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/service/time/time.h"
#include "core/hle/service/time/time_s.h"
#include "core/hle/service/time/time_u.h"

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
            {2, &ITimeZoneService::GetTotalLocationNameCount, "GetTotalLocationNameCount"},
            {101, &ITimeZoneService::ToCalendarTimeWithMyRule, "ToCalendarTimeWithMyRule"},
        };
        RegisterHandlers(functions);
    }

private:
    void GetDeviceLocationName(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service, "(STUBBED) called");
        LocationName location_name{};
        IPC::RequestBuilder rb{ctx, (sizeof(LocationName) / 4) + 2};
        rb.Push(RESULT_SUCCESS);
        rb.PushRaw(location_name);
    }

    void GetTotalLocationNameCount(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service, "(STUBBED) called");
        IPC::RequestBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(0);
    }

    void ToCalendarTimeWithMyRule(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        u64 posix_time = rp.Pop<u64>();

        LOG_WARNING(Service, "(STUBBED) called, posix_time=0x%016llX", posix_time);

        CalendarTime calendar_time{2018, 1, 1, 0, 0, 0};
        CalendarAdditionalInfo additional_info{};
        IPC::RequestBuilder rb{ctx, 10};
        rb.Push(RESULT_SUCCESS);
        rb.PushRaw(calendar_time);
        rb.PushRaw(additional_info);
    }
};

void Module::Interface::GetStandardUserSystemClock(Kernel::HLERequestContext& ctx) {
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

void Module::Interface::GetStandardNetworkSystemClock(Kernel::HLERequestContext& ctx) {
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

void Module::Interface::GetStandardSteadyClock(Kernel::HLERequestContext& ctx) {
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

void Module::Interface::GetTimeZoneService(Kernel::HLERequestContext& ctx) {
    IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<ITimeZoneService>();
    LOG_DEBUG(Service, "called");
}

Module::Interface::Interface(std::shared_ptr<Module> time, const char* name)
    : ServiceFramework(name), time(std::move(time)) {}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    auto time = std::make_shared<Module>();
    std::make_shared<TIME_S>(time)->InstallAsService(service_manager);
    std::make_shared<TIME_U>(time)->InstallAsService(service_manager);
}

} // namespace Time
} // namespace Service
