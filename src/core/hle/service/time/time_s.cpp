// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <chrono>
#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/service/time/time_s.h"

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

void TimeS::GetStandardUserSystemClock(Kernel::HLERequestContext& ctx) {
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

TimeS::TimeS() : ServiceFramework("time:s") {
    static const FunctionInfo functions[] = {
        {0x00000000, &TimeS::GetStandardUserSystemClock, "GetStandardUserSystemClock"},
    };
    RegisterHandlers(functions);
}

} // namespace Time
} // namespace Service
