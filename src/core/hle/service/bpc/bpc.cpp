// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>

#include "core/hle/service/bpc/bpc.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

namespace Service::BPC {

class BPC final : public ServiceFramework<BPC> {
public:
    explicit BPC() : ServiceFramework{"bpc"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "ShutdownSystem"},
            {1, nullptr, "RebootSystem"},
            {2, nullptr, "GetWakeupReason"},
            {3, nullptr, "GetShutdownReason"},
            {4, nullptr, "GetAcOk"},
            {5, nullptr, "GetBoardPowerControlEvent"},
            {6, nullptr, "GetSleepButtonState"}, // 2.0.0+
            {7, nullptr, "GetPowerEvent"}, // 2.0.0+
            {8, nullptr, "CreateWakeupTimer"}, // 2.0.0 - 9.2.0
            {9, nullptr, "CancelWakeupTimer"}, // 2.0.0 - 9.2.0
            {10, nullptr, "EnableWakeupTimerOnDevice"}, // 2.0.0 - 9.2.0
            {11, nullptr, "CreateWakeupTimerEx"}, // 3.0.0 - 9.2.0
            {12, nullptr, "GetLastEnabledWakeupTimerType"}, // 3.0.0 - 9.2.0
            {13, nullptr, "CleanAllWakeupTimers"}, // 3.0.0 - 9.2.0
            {14, nullptr, "Unknown"}, // 6.0.0+
            {15, nullptr, "Unknown2"}, // 10.0.0+
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class BPC_R final : public ServiceFramework<BPC_R> {
public:
    explicit BPC_R() : ServiceFramework{"bpc:r"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetRtcTime"},
            {1, nullptr, "SetRtcTime"},
            {2, nullptr, "GetRtcResetDetected"},
            {3, nullptr, "ClearRtcResetDetected"}, // 2.0.0+
            {4, nullptr, "SetUpRtcResetOnShutdown"}, // 3.0.0+
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void InstallInterfaces(SM::ServiceManager& sm) {
    std::make_shared<BPC>()->InstallAsService(sm);
    std::make_shared<BPC_R>()->InstallAsService(sm);
}

} // namespace Service::BPC
