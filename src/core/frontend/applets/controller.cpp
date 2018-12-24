// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/frontend/applets/controller.h"
#include "core/hle/service/hid/controllers/npad.h"
#include "core/hle/service/hid/hid.h"
#include "core/hle/service/sm/sm.h"

namespace Core::Frontend {

ControllerApplet::~ControllerApplet() = default;

DefaultControllerApplet::~DefaultControllerApplet() = default;

void DefaultControllerApplet::ReconfigureControllers(std::function<void(bool)> completed,
                                                     ControllerParameters parameters) const {
    LOG_WARNING(Service_HID, "(STUBBED) called, automatically setting controller types to best fit "
                             "in configuration, may be incorrect!");

    const auto& npad =
        Core::System::GetInstance()
            .ServiceManager()
            .GetService<Service::HID::Hid>("hid")
            ->GetAppletResource()
            ->GetController<Service::HID::Controller_NPad>(Service::HID::HidController::NPad);

    for (auto& player : Settings::values.players) {
        const auto prio = Service::HID::Controller_NPad::MapSettingsTypeToNPad(player.type);
        player.type =
            Service::HID::Controller_NPad::MapNPadTypeToSettings(npad.DecideBestController(prio));
    }

    completed(true);
}

} // namespace Core::Frontend
