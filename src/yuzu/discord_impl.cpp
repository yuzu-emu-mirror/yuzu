// SPDX-FileCopyrightText: 2018 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <string>
#include <discord_rpc.h>
#include <fmt/format.h>
#include "common/common_types.h"
#include "core/core.h"
#include "core/loader/loader.h"
#include "yuzu/discord_impl.h"
#include "yuzu/uisettings.h"

namespace DiscordRPC {

DiscordImpl::DiscordImpl(Core::System& system_) : system{system_} {
    DiscordEventHandlers handlers{};

    // The number is the client ID for yuzu, it's used for images and the
    // application name
    Discord_Initialize("712465656758665259", &handlers, 1, nullptr);
}

DiscordImpl::~DiscordImpl() {
    Discord_ClearPresence();
    Discord_Shutdown();
}

void DiscordImpl::Pause() {
    Discord_ClearPresence();
}

void DiscordImpl::Update() {
    s64 start_time = std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    std::string title;
    u64 titleId;

    if (system.IsPoweredOn()) {
        system.GetAppLoader().ReadTitle(title);
        system.GetAppLoader().ReadProgramId(titleId);
    }

    DiscordRichPresence presence{};

    if (system.IsPoweredOn()) {
        std::string gameCoverUrl = "https://tinfoil.media/ti/";
        gameCoverUrl += QString::fromStdString(fmt::format("{:016X}", titleId)).toStdString();
        gameCoverUrl += "/512/512";

        presence.largeImageKey = gameCoverUrl.c_str();
        presence.largeImageText = title.c_str();

        presence.smallImageKey = "yuzu_logo";
        presence.smallImageText = "yuzu is an emulator for the Nintendo Switch";

        presence.state = title.c_str();
        presence.details = "Currently in game";
    } else {
        presence.largeImageKey = "yuzu_logo";
        presence.largeImageText = "yuzu is an emulator for the Nintendo Switch";
        presence.details = "Not in game";
    }
    presence.startTimestamp = start_time;
    Discord_UpdatePresence(&presence);
}
} // namespace DiscordRPC
