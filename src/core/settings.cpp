// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/gdbstub/gdbstub.h"
#include "core/hle/service/hid/hid.h"
#include "core/settings.h"
#include "video_core/renderer_base.h"

namespace Settings {

Values values = {};

Values::Values() : current_game(default_game) {}

void Values::SetUpdateCurrentGameFunction(std::function<bool(u64, PerGameValues&)> new_function) {
    update_current_game = std::move(new_function);
}

u64 Values::CurrentTitleID() {
    return current_title_id;
}

void Values::SetCurrentTitleID(u64 title_id) {
    // Ignore Updates
    title_id &= ~0x800;

    if (current_title_id == title_id)
        return;

    update_current_game(title_id, current_game);
}

PerGameValues& Values::operator[](u64 title_id) {
    // Ignore Updates
    title_id &= ~0x800;

    if (current_title_id == title_id)
        return current_game;

    if (!update_current_game(title_id, current_game))
        return default_game;

    current_title_id = title_id;
    return current_game;
}

const PerGameValues& Values::operator[](u64 title_id) const {
    // Ignore Updates
    title_id &= ~0x800;

    if (current_title_id == title_id)
        return current_game;

    return default_game;
}

PerGameValues* Values::operator->() {
    if (current_title_id == 0)
        return &default_game;

    return &current_game;
}

const PerGameValues* Values::operator->() const {
    if (current_title_id == 0)
        return &default_game;

    return &current_game;
}

PerGameValues& Values::operator*() {
    if (current_title_id == 0)
        return default_game;
    return current_game;
}

const PerGameValues& Values::operator*() const {
    if (current_title_id == 0)
        return default_game;
    return current_game;
}

void Apply() {
    GDBStub::SetServerPort(values.gdbstub_port);
    GDBStub::ToggleServer(values.use_gdbstub);

    auto& system_instance = Core::System::GetInstance();
    if (system_instance.IsPoweredOn()) {
        system_instance.Renderer().RefreshBaseSettings();
    }

    Service::HID::ReloadInputDevices();
}

} // namespace Settings
