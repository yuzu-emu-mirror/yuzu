// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/ipc_helpers.h"
#include "core/hle/service/sockets/bsd_u.h"

namespace Service {
namespace Sockets {

void BSD_U::RegisterClient(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};

    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0); // bsd errno
}

void BSD_U::Socket(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    u32 domain = rp.Pop<u32>();
    u32 type = rp.Pop<u32>();
    u32 protocol = rp.Pop<u32>();

    LOG_WARNING(Service, "(STUBBED) called domain=%u type=%u protocol=%u", domain, type, protocol);

    u32 fd = next_fd++;

    IPC::ResponseBuilder rb{ctx, 4};

    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(fd);
    rb.Push<u32>(0); // bsd errno
}

void BSD_U::Connect(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 4};

    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0); // ret
    rb.Push<u32>(0); // bsd errno
}

void BSD_U::SendTo(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 4};

    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0); // ret
    rb.Push<u32>(0); // bsd errno
}

BSD_U::BSD_U() : ServiceFramework("bsd:u") {
    static const FunctionInfo functions[] = {{0, &BSD_U::RegisterClient, "RegisterClient"},
                                             {2, &BSD_U::Socket, "Socket"},
                                             {11, &BSD_U::SendTo, "SendTo"},
                                             {14, &BSD_U::Connect, "Connect"}};
    RegisterHandlers(functions);
}

} // namespace Sockets
} // namespace Service
