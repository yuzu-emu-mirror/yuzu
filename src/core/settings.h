// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <string>
#include "common/common_types.h"

namespace Settings {

namespace NativeButton {
enum Values {
    A,
    B,
    X,
    Y,
    Up,
    Down,
    Left,
    Right,
    L,
    R,
    Start,
    Select,

    ZL,
    ZR,

    Home,

    NumButtons,
};

constexpr int BUTTON_HID_BEGIN = A;
constexpr int BUTTON_IR_BEGIN = ZL;
constexpr int BUTTON_NS_BEGIN = Home;

constexpr int BUTTON_HID_END = BUTTON_IR_BEGIN;
constexpr int BUTTON_IR_END = BUTTON_NS_BEGIN;
constexpr int BUTTON_NS_END = NumButtons;

constexpr int NUM_BUTTONS_HID = BUTTON_HID_END - BUTTON_HID_BEGIN;
constexpr int NUM_BUTTONS_IR = BUTTON_IR_END - BUTTON_IR_BEGIN;
constexpr int NUM_BUTTONS_NS = BUTTON_NS_END - BUTTON_NS_BEGIN;

static const std::array<const char*, NumButtons> mapping = {{
    "button_a", "button_b", "button_x", "button_y", "button_up", "button_down", "button_left",
    "button_right", "button_l", "button_r", "button_start", "button_select", "button_zl",
    "button_zr", "button_home",
}};
} // namespace NativeButton

namespace NativeAnalog {
enum Values {
    CirclePad,
    CStick,

    NumAnalogs,
};

static const std::array<const char*, NumAnalogs> mapping = {{
    "circle_pad", "c_stick",
}};
} // namespace NativeAnalog

enum class CpuCore {
    Unicorn,
    Dynarmic,
};

struct Values {
    // Controls
    std::array<std::string, NativeButton::NumButtons> buttons;
    std::array<std::string, NativeAnalog::NumAnalogs> analogs;
    std::string motion_device;
    std::string touch_device;

    // Core
    CpuCore cpu_core;

    // Data Storage
    bool use_virtual_sd;

    // Renderer
    float resolution_factor;
    bool toggle_framelimit;

    float bg_red;
    float bg_green;
    float bg_blue;

    std::string log_filter;

    // Debugging
    bool use_gdbstub;
    u16 gdbstub_port;
} extern values;

void Apply();
} // namespace Settings
