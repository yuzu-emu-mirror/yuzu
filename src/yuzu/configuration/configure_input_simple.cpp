// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <tuple>

#include "ui_configure_input_simple.h"
#include "yuzu/configuration/configure_input.h"
#include "yuzu/configuration/configure_input_player.h"
#include "yuzu/configuration/configure_input_simple.h"
#include "yuzu/ui_settings.h"

namespace {

template <typename Dialog, typename... Args>
void CallConfigureDialog(ConfigureInputSimple* caller, Args&&... args) {
    caller->applyConfiguration();
    Dialog dialog(caller, std::forward<Args>(args)...);

    const auto res = dialog.exec();
    if (res == QDialog::Accepted) {
        dialog.applyConfiguration();
    }
}

// OnProfileSelect functions should (when applicable):
// - Set controller types
// - Set controller enabled
// - Set docked mode
// - Set advanced controller config/enabled (i.e. debug, kbd, mouse, touch)
//
// OnProfileSelect function should NOT however:
// - Reset any button mappings
// - Open any dialogs
// - Block in any way

constexpr std::size_t HANDHELD_INDEX = 8;

void HandheldOnProfileSelect() {
    Settings::values.players[HANDHELD_INDEX].connected = true;
    Settings::values.players[HANDHELD_INDEX].type = Settings::ControllerType::DualJoycon;

    for (std::size_t player = 0; player < HANDHELD_INDEX; ++player) {
        Settings::values.players[player].connected = false;
    }

    Settings::values.use_docked_mode = false;
    Settings::values.keyboard_enabled = false;
    Settings::values.mouse_enabled = false;
    Settings::values.debug_pad_enabled = false;
    Settings::values.touchscreen.enabled = true;
}

void DualJoyconsDockedOnProfileSelect() {
    Settings::values.players[0].connected = true;
    Settings::values.players[0].type = Settings::ControllerType::DualJoycon;

    for (std::size_t player = 1; player <= HANDHELD_INDEX; ++player) {
        Settings::values.players[player].connected = false;
    }

    Settings::values.use_docked_mode = true;
    Settings::values.keyboard_enabled = false;
    Settings::values.mouse_enabled = false;
    Settings::values.debug_pad_enabled = false;
    Settings::values.touchscreen.enabled = false;
}

// Name, OnProfileSelect (called when selected in drop down), OnConfigure (called when configure
// is clicked)
using InputProfile = std::tuple<const char*, void (*)(), void (*)(ConfigureInputSimple*)>;

constexpr std::array<InputProfile, 3> INPUT_PROFILES{{
    {QT_TR_NOOP("Single Player - Handheld - Undocked"), HandheldOnProfileSelect,
     [](ConfigureInputSimple* caller) {
         CallConfigureDialog<ConfigureInputPlayer>(caller, HANDHELD_INDEX, false);
     }},
    {QT_TR_NOOP("Single Player - Dual Joycons - Docked"), DualJoyconsDockedOnProfileSelect,
     [](ConfigureInputSimple* caller) {
         CallConfigureDialog<ConfigureInputPlayer>(caller, 1, false);
     }},
    {QT_TR_NOOP("Custom"), [] {}, CallConfigureDialog<ConfigureInput>},
}};

} // namespace

void ApplyInputProfileConfiguration(int profile_index) {
    std::get<1>(
        INPUT_PROFILES.at(std::min(profile_index, static_cast<int>(INPUT_PROFILES.size() - 1))))();
}

ConfigureInputSimple::ConfigureInputSimple(
    QWidget* parent, std::optional<Core::Frontend::ControllerParameters> constraints)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureInputSimple>()), constraints(constraints) {
    ui->setupUi(this);

    for (const auto& profile : INPUT_PROFILES) {
        const QString label = tr(std::get<0>(profile));
        ui->profile_combobox->addItem(label, label);
    }

    connect(ui->profile_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureInputSimple::OnSelectProfile);
    connect(ui->profile_configure, &QPushButton::pressed, this, &ConfigureInputSimple::OnConfigure);

    this->loadConfiguration();
    UpdateErrors();
}

ConfigureInputSimple::~ConfigureInputSimple() = default;

void ConfigureInputSimple::applyConfiguration() {
    auto index = ui->profile_combobox->currentIndex();
    // Make the stored index for "Custom" very large so that if new profiles are added it
    // doesn't change.
    if (index >= static_cast<int>(INPUT_PROFILES.size() - 1))
        index = std::numeric_limits<int>::max();

    UISettings::values.profile_index = index;
}

void ConfigureInputSimple::loadConfiguration() {
    const auto index = UISettings::values.profile_index;
    if (index >= static_cast<int>(INPUT_PROFILES.size()) || index < 0)
        ui->profile_combobox->setCurrentIndex(static_cast<int>(INPUT_PROFILES.size() - 1));
    else
        ui->profile_combobox->setCurrentIndex(index);
}

void ConfigureInputSimple::OnSelectProfile(int index) {
    const auto old_docked = Settings::values.use_docked_mode;
    ApplyInputProfileConfiguration(index);
    OnDockedModeChanged(old_docked, Settings::values.use_docked_mode);
    UpdateErrors();
}

void ConfigureInputSimple::OnConfigure() {
    std::get<2>(INPUT_PROFILES.at(ui->profile_combobox->currentIndex()))(this);
    UpdateErrors();
}

static bool AnyPlayersMatchingType(Settings::ControllerType type) {
    return std::any_of(
        Settings::values.players.begin(), Settings::values.players.begin() + 8,
        [type](const Settings::PlayerInput& in) { return in.type == type && in.connected; });
}

void ConfigureInputSimple::UpdateErrors() {
    if (constraints == std::nullopt) {
        ui->error_view->setHidden(true);
        return;
    }

    ui->error_view->setHidden(false);

    QString text;

    const auto number_player =
        std::count_if(Settings::values.players.begin(), Settings::values.players.begin() + 9,
                      [](const Settings::PlayerInput& in) { return in.connected; });

    if (number_player < constraints->min_players) {
        text += tr("<li>The game requires a <i>minimum</i> of %1 players, you currently have %2 "
                   "players.</li>")
                    .arg(constraints->min_players)
                    .arg(number_player);
    }

    if (number_player > constraints->max_players) {
        text += tr("<li>The game allows a <i>maximum</i> of %1 players, you currently have %2 "
                   "players.</li>")
                    .arg(constraints->max_players)
                    .arg(number_player);
    }

    if (AnyPlayersMatchingType(Settings::ControllerType::ProController) &&
        !constraints->allowed_pro_controller) {
        text += tr("<li>The game does not allow the use of the <i>Pro Controller</i>.</li>");
    }

    if (AnyPlayersMatchingType(Settings::ControllerType::DualJoycon) &&
        !constraints->allowed_joycon_dual) {
        text += tr("<li>The game does not allow the use of <i>Dual Joycons</i>.</li>");
    }

    if (AnyPlayersMatchingType(Settings::ControllerType::LeftJoycon) &&
        !constraints->allowed_joycon_left) {
        text += tr("<li>The game does not allow the use of the <i>Single Left Joycon</i> "
                   "controller.</li>");
    }

    if (AnyPlayersMatchingType(Settings::ControllerType::RightJoycon) &&
        !constraints->allowed_joycon_right) {
        text += tr("<li>The game does not allow the use of the <i>Single Right Joycon</i> "
                   "controller.</li>");
    }

    if (Settings::values.players[HANDHELD_INDEX].connected && !constraints->allowed_handheld) {
        text += tr("<li>The game does not allow the use of the <i>Handheld</i> Controller.</li>");
    }

    const auto has_single = AnyPlayersMatchingType(Settings::ControllerType::LeftJoycon) ||
                            AnyPlayersMatchingType(Settings::ControllerType::RightJoycon);

    if (has_single && !constraints->allowed_single_layout) {
        text += tr("<li>The game does not allow <i>single joycon</i> controllers.</li>");
    }

    if (has_single && constraints->merge_dual_joycons) {
        text += tr(
            "<li>The game requires that pairs of <i>single joycons</i> be merged into one <i>Dual "
            "Joycon</i> controller.</li>");
    }

    if (!text.isEmpty()) {
        is_valid = false;
        ui->error_view->setText(
            "<h4 style=\"color:#CC0000;\">The following errors were identified in "
            "your input configuration:</h4><ul>" +
            text + "</ul>");
    } else {
        is_valid = true;
        ui->error_view->setText(
            tr("<h4 style=\"color:#00CC00;\">Your input configuration is valid.</h4>"));
    }
}
