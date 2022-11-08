// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>
#include "common/announce_multiplayer_room.h"
#include "common/common_types.h"
#include "common/socket_types.h"
#include "network/verify_user.h"

namespace Network {
class RoomPostOffice final {
public:
    enum class State : u8 {
        Open,    ///< The room post office is open and ready to accept connections.
        Closed,  ///< The room post office is not opened and can not accept connections.
    };

    RoomPostOffice();
    ~RoomPostOffice();

    /**
    * Gets the current state of the room post office.
    */
    State GetState() const;

    /**
    * Creates the socket for this room post office.
    * Will bind to default address if server is empty string.
    */
    bool Create(const std::string& server_addr, u16 server_port);

    /**
    * Destorys the socket.
    */
    bool Destroy();

private:
    class RoomPostOfficeImpl;
    std::unique_ptr<RoomPostOfficeImpl> room_post_office_impl;

};
}
