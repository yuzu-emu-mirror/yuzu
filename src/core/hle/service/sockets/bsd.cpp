// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/ipc_helpers.h"
#include "core/hle/service/sockets/bsd.h"

namespace Service {
namespace Sockets {

void BSD::RegisterClient(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};

    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0); // bsd errno
}

void BSD::StartMonitoring(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};

    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0); // bsd errno
}

void BSD::Socket(Kernel::HLERequestContext& ctx) {
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

void BSD::Connect(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 4};

    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0); // ret
    rb.Push<u32>(0); // bsd errno
}

void BSD::SendTo(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 4};

    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0); // ret
    rb.Push<u32>(0); // bsd errno
}

void BSD::Close(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 4};

    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(0); // ret
    rb.Push<u32>(0); // bsd errno
}

BSD::BSD(const char* name) : ServiceFramework(name) {
    static const FunctionInfo functions[] = {
        {0, &BSD::RegisterClient, "RegisterClient"},
        {1, &BSD::StartMonitoring, "StartMonitoring"},
        {2, &BSD::Socket, "Socket"},
        {11, &BSD::SendTo, "SendTo"},
        {14, &BSD::Connect, "Connect"},
        {26, &BSD::Close, "Close"},
    };
    RegisterHandlers(functions);
}

} // namespace Sockets
} // namespace Service
