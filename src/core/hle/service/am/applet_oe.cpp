// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/am/applet_oe.h"

namespace Service {
namespace AM {

AppletOE::AppletOE() : ServiceFramework("appletOE") {
    static const FunctionInfo functions[] = {
        {0x00000000, nullptr, "OpenApplicationProxy"},
    };
    RegisterHandlers(functions);
}

AppletOE::~AppletOE() = default;

} // namespace AM
} // namespace Service
