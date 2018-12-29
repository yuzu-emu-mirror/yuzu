// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/frontend/applets/controller.h"
#include "core/hle/service/am/applets/controller.h"
#include "core/hle/service/hid/controllers/npad.h"

namespace Service::AM::Applets {

static Core::Frontend::ControllerParameters ConvertToFrontendParameters(
    ControllerConfigPrivate config_private, ControllerConfig config) {
    Core::Frontend::ControllerParameters params{};

    params.min_players = config.minimum_player_count;
    params.max_players = config.maximum_player_count;
    params.keep_current_connected = config.keep_current_connections;
    params.merge_dual_joycons = config.permit_dual_joycons;
    params.allowed_single_layout = config.enable_single_mode;

    params.horizontal_single_joycons = config_private.npad_hold_type;

    HID::Controller_NPad::NPadType styleset;
    styleset.raw = config_private.npad_style_set;
    params.allowed_pro_controller = styleset.pro_controller;
    params.allowed_handheld = styleset.handheld;
    params.allowed_joycon_dual = styleset.joycon_dual;
    params.allowed_joycon_left = styleset.joycon_left;
    params.allowed_joycon_right = styleset.joycon_right;

    return params;
}

Controller::Controller() = default;

Controller::~Controller() = default;

void Controller::Initialize() {
    Applet::Initialize();

    const auto private_config_storage = broker.PopNormalDataToApplet();
    ASSERT(private_config_storage != nullptr);
    const auto& private_controller_config = private_config_storage->GetData();

    ASSERT(private_controller_config.size() >= sizeof(ControllerConfigPrivate));
    std::memcpy(&private_config, private_controller_config.data(), sizeof(ControllerConfigPrivate));

    const auto config_storage = broker.PopNormalDataToApplet();
    ASSERT(config_storage != nullptr);
    const auto& controller_config = config_storage->GetData();

    ASSERT(controller_config.size() >= sizeof(ControllerConfig));
    std::memcpy(&config, controller_config.data(), sizeof(ControllerConfig));
}

bool Controller::TransactionComplete() const {
    return complete;
}

ResultCode Controller::GetStatus() const {
    return RESULT_SUCCESS;
}

void Controller::ExecuteInteractive() {
    UNREACHABLE_MSG("Tried to interact with non-interactable applet.");
}

void Controller::Execute() {
    if (complete) {
        UNREACHABLE();
    }

    const auto& frontend{Core::System::GetInstance().GetControllerApplet()};

    const auto parameters = ConvertToFrontendParameters(private_config, config);

    frontend.ReconfigureControllers([this](bool ok) { ConfigurationComplete(ok); }, parameters);
}

void Controller::ConfigurationComplete(bool ok) {
    ControllerConfigOutput out{};

    out.player_code = 0;
    out.selected_npad_id = static_cast<u32>(
        HID::Controller_NPad::MapSettingsTypeToNPad(Settings::values.players[0].type));
    out.result = !ok;

    complete = true;

    std::vector<u8> out_data(sizeof(ControllerConfigOutput));
    std::memcpy(out_data.data(), &out, sizeof(ControllerConfigOutput));
    broker.PushNormalDataFromApplet(IStorage{out_data});
    broker.SignalStateChanged();
}
} // namespace Service::AM::Applets
