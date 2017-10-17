// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <fmt/format.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/hle/ipc.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/server_port.h"
#include "core/hle/kernel/server_session.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/apm/apm.h"
#include "core/hle/service/dsp_dsp.h"
#include "core/hle/service/gsp_gpu.h"
#include "core/hle/service/hid/hid.h"
#include "core/hle/service/lm/lm.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/controller.h"
#include "core/hle/service/sm/sm.h"

using Kernel::ClientPort;
using Kernel::ServerPort;
using Kernel::ServerSession;
using Kernel::SharedPtr;

namespace Service {

std::unordered_map<std::string, SharedPtr<ClientPort>> g_kernel_named_ports;

/**
 * Creates a function string for logging, complete with the name (or header code, depending
 * on what's passed in) the port name, and all the cmd_buff arguments.
 */
static std::string MakeFunctionString(const char* name, const char* port_name,
                                      const u32* cmd_buff) {
    // Number of params == bits 0-5 + bits 6-11
    int num_params = (cmd_buff[0] & 0x3F) + ((cmd_buff[0] >> 6) & 0x3F);

    std::string function_string =
        Common::StringFromFormat("function '%s': port=%s", name, port_name);
    for (int i = 1; i <= num_params; ++i) {
        function_string += Common::StringFromFormat(", cmd_buff[%i]=0x%X", i, cmd_buff[i]);
    }
    return function_string;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ServiceFrameworkBase::ServiceFrameworkBase(const char* service_name, u32 max_sessions,
                                           InvokerFn* handler_invoker)
    : service_name(service_name), max_sessions(max_sessions), handler_invoker(handler_invoker) {}

ServiceFrameworkBase::~ServiceFrameworkBase() = default;

void ServiceFrameworkBase::InstallAsService(SM::ServiceManager& service_manager) {
    ASSERT(port == nullptr);
    port = service_manager.RegisterService(service_name, max_sessions).Unwrap();
    port->SetHleHandler(shared_from_this());
}

void ServiceFrameworkBase::InstallAsNamedPort() {
    ASSERT(port == nullptr);
    SharedPtr<ServerPort> server_port;
    SharedPtr<ClientPort> client_port;
    std::tie(server_port, client_port) = ServerPort::CreatePortPair(max_sessions, service_name);
    server_port->SetHleHandler(shared_from_this());
    AddNamedPort(service_name, std::move(client_port));
}

void ServiceFrameworkBase::RegisterHandlersBase(const FunctionInfoBase* functions, size_t n) {
    handlers.reserve(handlers.size() + n);
    for (size_t i = 0; i < n; ++i) {
        // Usually this array is sorted by id already, so hint to insert at the end
        handlers.emplace_hint(handlers.cend(), functions[i].expected_header, functions[i]);
    }
}

void ServiceFrameworkBase::ReportUnimplementedFunction(Kernel::HLERequestContext& ctx,
                                                       const FunctionInfoBase* info) {
    auto cmd_buf = ctx.CommandBuffer();
    std::string function_name = info == nullptr ? fmt::format("{:#08x}", ctx.GetCommand()) : info->name;

    fmt::MemoryWriter w;
    w.write("function '{}': port='{}' cmd_buf={{[0]={:#x}", function_name, service_name,
            cmd_buf[0]);
    for (int i = 1; i <= 8; ++i) {
        w.write(", [{}]={:#x}", i, cmd_buf[i]);
    }
    w << '}';

    LOG_ERROR(Service, "unknown / unimplemented %s", w.c_str());
    // TODO(bunnei): Hack - ignore error
    IPC::RequestBuilder rb{ctx, 1};
    rb.Push(RESULT_SUCCESS);
}

void ServiceFrameworkBase::InvokeRequest(Kernel::HLERequestContext& ctx) {
    auto itr = handlers.find(ctx.GetCommand());
    const FunctionInfoBase* info = itr == handlers.end() ? nullptr : &itr->second;
    if (info == nullptr || info->handler_callback == nullptr) {
        return ReportUnimplementedFunction(ctx, info);
    }

    LOG_TRACE(
        Service, "%s",
        MakeFunctionString(info->name, GetServiceName().c_str(), ctx.CommandBuffer()).c_str());
    handler_invoker(this, info->handler_callback, ctx);
}

void ServiceFrameworkBase::HandleSyncRequest(SharedPtr<ServerSession> server_session) {
    u32* cmd_buf = (u32*)Memory::GetPointer(Kernel::GetCurrentThread()->GetTLSAddress());

    // TODO(yuriks): The kernel should be the one handling this as part of translation after
    // everything else is migrated
    Kernel::HLERequestContext context(std::move(server_session));
    context.PopulateFromIncomingCommandBuffer(cmd_buf, *Kernel::g_current_process,
                                              Kernel::g_handle_table);

    switch (context.GetCommandType()) {
    case IPC::CommandType::Close: {
        IPC::RequestBuilder rb{context, 1};
        rb.Push(RESULT_SUCCESS);
        break;
    }
    case IPC::CommandType::Control: {
        SM::g_service_manager->InvokeControlRequest(context);
        break;
    }
    case IPC::CommandType::Request: {
        InvokeRequest(context);
        break;
    }
    default:
        UNIMPLEMENTED_MSG("command_type=%d", context.GetCommandType());
    }

    context.WriteToOutgoingCommandBuffer(cmd_buf, *Kernel::g_current_process,
                                         Kernel::g_handle_table);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Module interface

// TODO(yuriks): Move to kernel
void AddNamedPort(std::string name, SharedPtr<ClientPort> port) {
    g_kernel_named_ports.emplace(std::move(name), std::move(port));
}

/// Initialize ServiceManager
void Init() {
    SM::g_service_manager = std::make_shared<SM::ServiceManager>();
    SM::ServiceManager::InstallInterfaces(SM::g_service_manager);

    AM::InstallInterfaces(*SM::g_service_manager);
    APM::InstallInterfaces(*SM::g_service_manager);
    LM::InstallInterfaces(*SM::g_service_manager);

    HID::Init();

    LOG_DEBUG(Service, "initialized OK");
}

/// Shutdown ServiceManager
void Shutdown() {
    HID::Shutdown();

    SM::g_service_manager = nullptr;
    g_kernel_named_ports.clear();
    LOG_DEBUG(Service, "shutdown OK");
}
} // namespace Service
