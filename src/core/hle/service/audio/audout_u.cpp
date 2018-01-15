// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/audio/audout_u.h"

namespace Service {
namespace Audio {

void AudOutU::ListAudioOuts(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::RequestBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

AudOutU::AudOutU() : ServiceFramework("audout:u") {
    static const FunctionInfo functions[] = {
        {0x00000000, &AudOutU::ListAudioOuts, "ListAudioOuts"},
    };
    RegisterHandlers(functions);
}

} // namespace Audio
} // namespace Service
