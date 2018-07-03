// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/audio/hwopus.h"

namespace Service::Audio {

void HwOpus::GetWorkBufferSize(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_Audio, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0x4000);
}

HwOpus::HwOpus() : ServiceFramework("hwopus") {
    static const FunctionInfo functions[] = {
        {0, nullptr, "Initialize"},
        {1, &HwOpus::GetWorkBufferSize, "GetWorkBufferSize"},
        {2, nullptr, "InitializeMultiStream"},
        {3, nullptr, "GetWorkBufferSizeMultiStream"},
    };
    RegisterHandlers(functions);
}

} // namespace Service::Audio
