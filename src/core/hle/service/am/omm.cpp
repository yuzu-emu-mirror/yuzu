// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/am/omm.h"

namespace Service::AM {

OMM::OMM() : ServiceFramework{"omm"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetOperationMode"},
        {1, nullptr, "GetOperationModeChangeEvent"},
        {2, nullptr, "EnableAudioVisual"},
        {3, nullptr, "DisableAudioVisual"},
        {4, nullptr, "EnterSleepAndWait"},
        {5, nullptr, "GetCradleStatus"},
        {6, nullptr, "FadeInDisplay"},
        {7, nullptr, "FadeOutDisplay"},
        {8, nullptr, "GetCradleFwVersion"}, // 2.0.0+
        {9, nullptr, "NotifyCecSettingsChanged"}, // 2.0.0+
        {10, nullptr, "SetOperationModePolicy"}, // 3.0.0+
        {11, nullptr, "GetDefaultDisplayResolution"}, // 3.0.0+
        {12, nullptr, "GetDefaultDisplayResolutionChangeEvent"}, // 3.0.0+
        {13, nullptr, "UpdateDefaultDisplayResolution"}, // 3.0.0+
        {14, nullptr, "ShouldSleepOnBoot"}, // 3.0.0+
        {15, nullptr, "NotifyHdcpApplicationExecutionStarted"}, // 4.0.0+
        {16, nullptr, "NotifyHdcpApplicationExecutionFinished"}, // 4.0.0+
        {17, nullptr, "NotifyHdcpApplicationDrawingStarted"}, // 4.0.0+
        {18, nullptr, "NotifyHdcpApplicationDrawingFinished"}, // 4.0.0+
        {19, nullptr, "GetHdcpAuthenticationFailedEvent"}, // 4.0.0+
        {20, nullptr, "GetHdcpAuthenticationFailedEmulationEnabled"}, // 4.0.0+
        {21, nullptr, "SetHdcpAuthenticationFailedEmulation"}, // 4.0.0+
        {22, nullptr, "GetHdcpStateChangeEvent"}, // 4.0.0+
        {23, nullptr, "GetHdcpState"}, // 4.0.0+
        {24, nullptr, "ShowCardUpdateProcessing"}, // 5.0.0+
        {25, nullptr, "SetApplicationCecSettingsAndNotifyChanged"}, // 5.0.0+
        {26, nullptr, "GetOperationModeSystemInfo"}, // 7.0.0+
        {27, nullptr, "GetAppletFullAwakingSystemEvent"}, // 9.0.0+
    };
    // clang-format on

    RegisterHandlers(functions);
}

OMM::~OMM() = default;

} // namespace Service::AM
