// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <iomanip>
#include <mutex>
#include <random>
#include <regex>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include "common/logging/log.h"
#include "enet/enet.h"
#include "network/packet.h"
#include "network/room.h"
#include "network/room_post_office.h"

namespace Network {
class RoomPostOffice::RoomPostOfficeImpl {
public:
    ENetHost* server = nullptr;
    /// Thread that receives and dispatches network packets
    std::unique_ptr<std::thread> room_center_thread;

    std::atomic<State> state{State::Closed};

    class Mailbox {
    public:
        ENetPeer* peer;  ///< the remote room's peer
    };
    using MailboxList = std::vector<Mailbox>;
    /// remote rooms
    MailboxList mailboxes;

    RoomPostOfficeImpl() {}

    /// Thread function that will receive and dispatch messages until the room is destroyed.
    void ServerLoop();
    void StartLoop();

    /**
     * Handle client disconnection.
     */
    void HandleClientDisconnection(const ENetPeer* peer);
    /**
     * Handle mail.
     */
    void HandleMail(const ENetEvent* event);
    /**
     * Handle the room joined room post office.
     */
    void HandleJoinMailbox(const ENetEvent* event);
};

RoomPostOffice::RoomPostOffice() : room_post_office_impl{std::make_unique<RoomPostOfficeImpl>()} {}
RoomPostOffice::~RoomPostOffice() = default;

void RoomPostOffice::RoomPostOfficeImpl::ServerLoop() {
    while (state != State::Closed) {
        ENetEvent event;
        if (enet_host_service(server, &event, 5) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                HandleMail(&event);
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                HandleClientDisconnection(event.peer);
                break;
            case ENET_EVENT_TYPE_NONE:
                break;
            case ENET_EVENT_TYPE_CONNECT:
                HandleJoinMailbox(&event);
                break;
            }
        }
    }

    for (const Mailbox& mailbox : mailboxes) {
        enet_peer_disconnect(mailbox.peer, 0);
    }
}

bool RoomPostOffice::Create(const std::string& server_address, u16 server_port) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    if (!server_address.empty()) {
        enet_address_set_host(&address, server_address.c_str());
    }
    address.port = server_port;
    room_post_office_impl->server = enet_host_create(&address, 16, NumChannels, 0, 0);
    if (!room_post_office_impl->server) {
        return false;
    }
    room_post_office_impl->state = State::Open;
    room_post_office_impl->StartLoop();
    return true;
}

bool RoomPostOffice::Destroy() {
    room_post_office_impl->state = State::Closed;
    room_post_office_impl->room_center_thread->join();
    room_post_office_impl->room_center_thread.reset();
    if (room_post_office_impl->server) {
        enet_host_destroy(room_post_office_impl->server);
    }
    room_post_office_impl->server = nullptr;
    room_post_office_impl->mailboxes.clear();
    return true;
}

void RoomPostOffice::RoomPostOfficeImpl::StartLoop() {
    room_center_thread = std::make_unique<std::thread>(&RoomPostOffice::RoomPostOfficeImpl::ServerLoop, this);
}

void RoomPostOffice::RoomPostOfficeImpl::HandleClientDisconnection(const ENetPeer* peer) {
    if (!peer) {
        return;
    }
    const auto& mailbox =
        std::find_if(mailboxes.begin(), mailboxes.end(), [peer](const Mailbox& mailbox_entry) {
            return mailbox_entry.peer == peer;
        });
    if (mailbox != mailboxes.end()) {
        mailboxes.erase(mailbox);
        LOG_INFO(Network, "some room leave to this room center");
    }
}

void RoomPostOffice::RoomPostOfficeImpl::HandleMail(const ENetEvent* event) {
    for (const auto& mailbox : mailboxes) {
        if (mailbox.peer != event->peer) {
            enet_peer_send(mailbox.peer, 0, event->packet);
        }
    }
    enet_host_flush(server);
}

void RoomPostOffice::RoomPostOfficeImpl::HandleJoinMailbox(const ENetEvent* event) {
    if (!event || !event->peer) {
        return;
    }
    Mailbox mailbox{};
    mailbox.peer = event->peer;

    const auto& result =
        std::find_if(mailboxes.begin(), mailboxes.end(), [mailbox] (const Mailbox& mailbox_entry) {
           return mailbox_entry.peer == mailbox.peer;
        });
    if (result == mailboxes.end()) {
        mailboxes.push_back(mailbox);
        ENetAddress address = event->peer->address;
        LOG_INFO(Network,
                "{}.{}.{}.{}:{} Join to this room post office",
                (address.host      ) & 0xFF,(address.host >>  8) & 0xFF,
                (address.host >> 16) & 0xFF,(address.host >> 24) & 0xFF,address.port);
    }
}

RoomPostOffice::State RoomPostOffice::GetState() const {
    return room_post_office_impl->state;
}

}
