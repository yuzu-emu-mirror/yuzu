// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <optional>
#include <string>
#include "common/bit_field.h"
#include "common/common_types.h"

namespace Core::Frontend {

struct ControllerParameters {
    u8 min_players;
    u8 max_players;
    bool keep_current_connected;
    bool merge_dual_joycons;
    bool allowed_single_layout;

    bool allowed_pro_controller;
    bool allowed_handheld;
    bool allowed_joycon_dual;
    bool allowed_joycon_left;
    bool allowed_joycon_right;

    bool horizontal_single_joycons;
};

class ControllerApplet {
public:
    virtual ~ControllerApplet();

    virtual void ReconfigureControllers(std::function<void(bool)> completed,
                                        ControllerParameters parameters) const = 0;
};

class DefaultControllerApplet final : public ControllerApplet {
public:
    ~DefaultControllerApplet() override;

    void ReconfigureControllers(std::function<void(bool)> completed,
                                ControllerParameters parameters) const override;
};

} // namespace Core::Frontend
