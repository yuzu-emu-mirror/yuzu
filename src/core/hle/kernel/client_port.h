// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include "common/common_types.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel {

class ServerPort;
class ServerSession;

class ClientPort final : public Object {
public:
    friend class ServerPort;
    std::string GetTypeName() const override {
        return "ClientPort";
    }
    std::string GetName() const override {
        return name;
    }

    static const HandleType HANDLE_TYPE = HandleType::ClientPort;
    HandleType GetHandleType() const override {
        return HANDLE_TYPE;
    }

    /**
     * Adds the specified server session to the queue of pending sessions of the associated ServerPort
     * @param server_session Server session to add to the queue
     */
    void AddWaitingSession(SharedPtr<ServerSession> server_session);

    SharedPtr<ServerPort> server_port; ///< ServerPort associated with this client port.
    u32 max_sessions;    ///< Maximum number of simultaneous sessions the port can have
    u32 active_sessions; ///< Number of currently open sessions to this port
    std::string name;    ///< Name of client port (optional)
private:
    ClientPort();
    ~ClientPort() override;
};

} // namespace
