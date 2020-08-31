// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <utility>

#include "common/assert.h"
#include "common/common_types.h"
#include "core/hle/service/sockets/sockets.h"
#include "core/hle/service/sockets/sockets_translate.h"
#include "core/network/network.h"

namespace Service::Sockets {

Errno Translate(Network::Errno value) {
    switch (value) {
    case Network::Errno::SUCCESS:
        return Errno::SUCCESS;
    case Network::Errno::BADF:
        return Errno::BADF;
    case Network::Errno::AGAIN:
        return Errno::AGAIN;
    case Network::Errno::INVAL:
        return Errno::INVAL;
    case Network::Errno::MFILE:
        return Errno::MFILE;
    case Network::Errno::NOTCONN:
        return Errno::NOTCONN;
    default:
        UNIMPLEMENTED_MSG("Unimplemented errno={}", static_cast<int>(value));
        return Errno::SUCCESS;
    }
}

std::pair<s32, Errno> Translate(std::pair<s32, Network::Errno> value) {
    return {value.first, Translate(value.second)};
}

Network::Domain Translate(Domain domain) {
    switch (domain) {
    case Domain::INET:
        return Network::Domain::INET;
    default:
        UNIMPLEMENTED_MSG("Unimplemented domain={}", static_cast<int>(domain));
        return {};
    }
}

Domain Translate(Network::Domain domain) {
    switch (domain) {
    case Network::Domain::INET:
        return Domain::INET;
    default:
        UNIMPLEMENTED_MSG("Unimplemented domain={}", static_cast<int>(domain));
        return {};
    }
}

Network::Type Translate(Type type) {
    switch (type) {
    case Type::STREAM:
        return Network::Type::STREAM;
    case Type::DGRAM:
        return Network::Type::DGRAM;
    default:
        UNIMPLEMENTED_MSG("Unimplemented type={}", static_cast<int>(type));
    }
}

Network::Protocol Translate(Type type, Protocol protocol) {
    switch (protocol) {
    case Protocol::UNSPECIFIED:
        LOG_WARNING(Service, "Unspecified protocol, assuming protocol from type");
        switch (type) {
        case Type::DGRAM:
            return Network::Protocol::UDP;
        case Type::STREAM:
            return Network::Protocol::TCP;
        default:
            return Network::Protocol::TCP;
        }
    case Protocol::TCP:
        return Network::Protocol::TCP;
    case Protocol::UDP:
        return Network::Protocol::UDP;
    default:
        UNIMPLEMENTED_MSG("Unimplemented protocol={}", static_cast<int>(protocol));
        return Network::Protocol::TCP;
    }
}

Network::PollEvents TranslatePollEventsToHost(PollEvents flags) {
    Network::PollEvents result{};
    const auto translate = [&result, &flags](PollEvents from, Network::PollEvents to) {
        if (True(flags & from)) {
            flags &= ~from;
            result |= to;
        }
    };
    translate(PollEvents::IN, Network::PollEvents::IN);
    translate(PollEvents::PRI, Network::PollEvents::PRI);
    translate(PollEvents::OUT, Network::PollEvents::OUT);
    translate(PollEvents::ERR, Network::PollEvents::ERR);
    translate(PollEvents::HUP, Network::PollEvents::HUP);
    translate(PollEvents::NVAL, Network::PollEvents::NVAL);

    UNIMPLEMENTED_IF_MSG((u16)flags != 0, "Unimplemented flags={}", (u16)flags);
    return result;
}

PollEvents TranslatePollEventsToGuest(Network::PollEvents flags) {
    PollEvents result{};
    const auto translate = [&result, &flags](Network::PollEvents from, PollEvents to) {
        if (True(flags & from)) {
            flags &= ~from;
            result |= to;
        }
    };

    translate(Network::PollEvents::IN, PollEvents::IN);
    translate(Network::PollEvents::PRI, PollEvents::PRI);
    translate(Network::PollEvents::OUT, PollEvents::OUT);
    translate(Network::PollEvents::ERR, PollEvents::ERR);
    translate(Network::PollEvents::HUP, PollEvents::HUP);
    translate(Network::PollEvents::NVAL, PollEvents::NVAL);

    UNIMPLEMENTED_IF_MSG((u16)flags != 0, "Unimplemented flags={}", (u16)flags);
    return result;
}

Network::SockAddrIn Translate(SockAddrIn value) {
    ASSERT(value.len == 0 || value.len == sizeof(value));

    return {
        .family = Translate(static_cast<Domain>(value.family)),
        .ip = value.ip,
        .portno = static_cast<u16>(value.portno >> 8 | value.portno << 8),
    };
}

SockAddrIn Translate(Network::SockAddrIn value) {
    return {
        .len = sizeof(SockAddrIn),
        .family = static_cast<u8>(Translate(value.family)),
        .portno = static_cast<u16>(value.portno >> 8 | value.portno << 8),
        .ip = value.ip,
        .zeroes = {},
    };
}

Network::ShutdownHow Translate(ShutdownHow how) {
    switch (how) {
    case ShutdownHow::RD:
        return Network::ShutdownHow::RD;
    case ShutdownHow::WR:
        return Network::ShutdownHow::WR;
    case ShutdownHow::RDWR:
        return Network::ShutdownHow::RDWR;
    default:
        UNIMPLEMENTED_MSG("Unimplemented how={}", static_cast<int>(how));
        return {};
    }
}

} // namespace Service::Sockets
