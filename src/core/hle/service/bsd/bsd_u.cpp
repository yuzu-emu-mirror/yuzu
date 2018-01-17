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

void BSD_U::Socket(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    u32 domain = rp.Pop<u32>();
    u32 type = rp.Pop<u32>();
    u32 protocol = rp.Pop<u32>();

    LOG_WARNING(Service, "(STUBBED) called domain=%d type=%d protocol=%d", domain, type, protocol);

    u32 fd = next_fd++;

    IPC::RequestBuilder rb{ctx, 3};

    rb.Push<u32>(fd);
    rb.Push<u32>(0); // bsd errno (no error)
}

BSD_U::BSD_U() : ServiceFramework("bsd:u") {
    static const FunctionInfo functions[] = {
        {0, &BSD_U::RegisterClient, "RegisterClient"},
        {2, &BSD_U::Socket, "Socket"}
    };
    RegisterHandlers(functions);
}

} // namespace BSD
} // namespace Service