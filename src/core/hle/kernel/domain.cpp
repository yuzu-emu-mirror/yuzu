// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/domain.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/session.h"
#include "core/hle/kernel/thread.h"

namespace Kernel {

ResultVal<SharedPtr<Domain>> Domain::Create(std::string name) {
    SharedPtr<Domain> domain(new Domain);
    domain->name = std::move(name);
    return MakeResult(std::move(domain));
}

ResultVal<SharedPtr<Domain>> Domain::CreateFromSession(const Session& session) {
    auto res = Create(session.port->GetName() + "_Domain");
    auto& domain = res.Unwrap();
    domain->request_handlers.push_back(std::move(session.server->hle_handler));
    Kernel::g_handle_table.ConvertSessionToDomain(session, domain);
    return res;
}

ResultCode Domain::SendSyncRequest(SharedPtr<Thread> thread) {
    Kernel::HLERequestContext context(this);
    u32* cmd_buf = (u32*)Memory::GetPointer(Kernel::GetCurrentThread()->GetTLSAddress());
    context.PopulateFromIncomingCommandBuffer(cmd_buf, *Kernel::g_current_process,
                                              Kernel::g_handle_table);

    auto& domain_message_header = context.GetDomainMessageHeader();
    if (domain_message_header) {
        // If there is a DomainMessageHeader, then this is CommandType "Request"
        const u32 object_id{context.GetDomainMessageHeader()->object_id};
        switch (domain_message_header->command) {
        case IPC::DomainMessageHeader::CommandType::SendMessage:
            return request_handlers[object_id - 1]->HandleSyncRequest(context);

        case IPC::DomainMessageHeader::CommandType::CloseVirtualHandle: {
            LOG_DEBUG(IPC, "CloseVirtualHandle, object_id=0x%08X", object_id);

            request_handlers[object_id - 1] = nullptr;

            IPC::RequestBuilder rb{context, 2};
            rb.Push(RESULT_SUCCESS);

            return RESULT_SUCCESS;
        }
        }

        LOG_CRITICAL(IPC, "Unknown domain command=%d", domain_message_header->command.Value());
        UNIMPLEMENTED();
    }
    return request_handlers.front()->HandleSyncRequest(context);
}

} // namespace Kernel
