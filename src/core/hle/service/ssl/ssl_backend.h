// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/result.h"

#include "common/common_types.h"

#include <memory>
#include <span>
#include <string>
#include <vector>

namespace Network {
class SocketBase;
}

namespace Service::SSL {

constexpr Result ResultNoSocket{ErrorModule::SSLSrv, 103};
constexpr Result ResultInvalidSocket{ErrorModule::SSLSrv, 106};
constexpr Result ResultTimeout{ErrorModule::SSLSrv, 205};
constexpr Result ResultInternalError{ErrorModule::SSLSrv, 999}; // made up

constexpr Result ResultWouldBlock{ErrorModule::SSLSrv, 204};
// ^ ResultWouldBlock is returned from Read and Write, and oddly, DoHandshake,
// with no way in the latter case to distinguish whether the client should poll
// for read or write.  The one official client I've seen handles this by always
// polling for read (with a timeout).

class SSLConnectionBackend {
public:
    virtual void SetSocket(std::shared_ptr<Network::SocketBase> socket) = 0;
    virtual Result SetHostName(const std::string& hostname) = 0;
    virtual Result DoHandshake() = 0;
    virtual ResultVal<size_t> Read(std::span<u8> data) = 0;
    virtual ResultVal<size_t> Write(std::span<const u8> data) = 0;
    virtual ResultVal<std::vector<std::vector<u8>>> GetServerCerts() = 0;
};

ResultVal<std::unique_ptr<SSLConnectionBackend>> CreateSSLConnectionBackend();

} // namespace Service::SSL
