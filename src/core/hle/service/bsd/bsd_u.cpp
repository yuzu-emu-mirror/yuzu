// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/ipc_helpers.h"
#include "core/hle/service/bsd/bsd_u.h"

namespace Service {
namespace BSD {

void BSD_U::RegisterClient(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::RequestBuilder rb{ctx, 3};

    // TODO(flerovium): first return value is BSD error number. Figure out what
    // the second return value means.
    rb.Push<u32>(0);
    rb.Push<u32>(0);
}

BSD_U::BSD_U() : ServiceFramework("bsd:u") {
    static const FunctionInfo functions[] = {
        {0, &BSD_U::RegisterClient, "RegisterClient"}
    };
    RegisterHandlers(functions);
}

} // namespace BSD
} // namespace Service