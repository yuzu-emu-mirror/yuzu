// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/server_port.h"
#include "core/hle/kernel/server_session.h"

namespace Kernel {

ClientPort::ClientPort() {}
ClientPort::~ClientPort() {}

SharedPtr<ClientSession> ClientPort::Connect() {
    // Create a new session pair, let the created sessions inherit the parent port's HLE handler.
    auto sessions = ServerSession::CreateSessionPair(server_port->GetName(), server_port->hle_handler);
    auto client_session = std::get<SharedPtr<ClientSession>>(sessions);
    auto server_session = std::get<SharedPtr<ServerSession>>(sessions);

    server_port->pending_sessions.push_back(server_session);

    // Wake the threads waiting on the ServerPort
    server_port->WakeupAllWaitingThreads();

    return std::move(client_session);
}

} // namespace
