// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/nvdrv/nvmemp.h"

namespace Service {
namespace Nvidia {

NVMEMP::NVMEMP() : ServiceFramework("nvmemp") {
    static const FunctionInfo functions[] = {
        {0, &NVMEMP::Unknown0, "Unknown0"},
        {1, &NVMEMP::Unknown1, "Unknown1"},
    };
    RegisterHandlers(functions);
}

void NVMEMP::Unknown0(Kernel::HLERequestContext& ctx) {
    UNIMPLEMENTED();
}

void NVMEMP::Unknown1(Kernel::HLERequestContext& ctx) {
    UNIMPLEMENTED();
}

} // namespace Nvidia
} // namespace Service
