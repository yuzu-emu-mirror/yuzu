// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace HID {

// Begin enums and output structs

enum ControllerType : u32 {
    ControllerType_ProController = 1 << 0,
    ControllerType_Handheld = 1 << 1,
    ControllerType_JoyconPair = 1 << 2,
    ControllerType_JoyconLeft = 1 << 3,
    ControllerType_JoyconRight = 1 << 4,
};

enum ControllerLayoutType : u32 {
    Layout_ProController = 0, // Pro Controller or HID gamepad
    Layout_Handheld = 1,      // Two Joy-Con docked to rails
    Layout_Single = 2, // Horizontal single Joy-Con or pair of Joy-Con, adjusted for orientation
    Layout_Left = 3,   // Only raw left Joy-Con state, no orientation adjustment
    Layout_Right = 4,  // Only raw right Joy-Con state, no orientation adjustment
    Layout_DefaultDigital = 5, // Same as next, but sticks have 8-direction values only
    Layout_Default = 6, // Safe default, single Joy-Con have buttons/sticks rotated for orientation
};

enum ControllerColorDescription {
    ColorDesc_ColorsNonexistent = 1 << 1,
};

enum ControllerConnectionState {
    ConnectionState_Connected = 1 << 0,
    ConnectionState_Wired = 1 << 1,
};

enum ControllerID {
    Controller_Player1 = 0,
    Controller_Player2 = 1,
    Controller_Player3 = 2,
    Controller_Player4 = 3,
    Controller_Player5 = 4,
    Controller_Player6 = 5,
    Controller_Player7 = 6,
    Controller_Player8 = 7,
    Controller_Handheld = 8,
    Controller_Unknown = 9,
};

// End enums and output structs

// Begin TouchScreen

struct TouchScreenHeader {
    u64 timestampTicks;
    u64 numEntries;
    u64 latestEntry;
    u64 maxEntryIndex;
    u64 timestamp;
};
static_assert(sizeof(TouchScreenHeader) == 0x28,
              "HID touch screen header structure has incorrect size");

struct TouchScreenEntryHeader {
    u64 timestamp;
    u64 numTouches;
};
static_assert(sizeof(TouchScreenEntryHeader) == 0x10,
              "HID touch screen entry header structure has incorrect size");

struct TouchScreenEntryTouch {
    u64 timestamp;
    u32 padding;
    u32 touchIndex;
    u32 x;
    u32 y;
    u32 diameterX;
    u32 diameterY;
    u32 angle;
    u32 padding_2;
};
static_assert(sizeof(TouchScreenEntryTouch) == 0x28,
              "HID touch screen touch structure has incorrect size");

struct TouchScreenEntry {
    TouchScreenEntryHeader header;
    std::array<TouchScreenEntryTouch, 16> touches;
    u64 unk;
};
static_assert(sizeof(TouchScreenEntry) == 0x298,
              "HID touch screen entry structure has incorrect size");

struct TouchScreen {
    TouchScreenHeader header;
    std::array<TouchScreenEntry, 17> entries;
    std::array<u8, 0x3c0> padding;
};
static_assert(sizeof(TouchScreen) == 0x3000, "HID touch screen structure has incorrect size");

// End TouchScreen

// Begin Mouse

struct MouseHeader {
    u64 timestampTicks;
    u64 numEntries;
    u64 latestEntry;
    u64 maxEntryIndex;
};
static_assert(sizeof(MouseHeader) == 0x20, "HID mouse header structure has incorrect size");

struct MouseButtonState {
    union {
        u64 hex{};

        // Buttons
        BitField<0, 1, u64> left;
        BitField<1, 1, u64> right;
        BitField<2, 1, u64> middle;
        BitField<3, 1, u64> forward;
        BitField<4, 1, u64> back;
    };
};

struct MouseEntry {
    u64 timestamp;
    u64 timestamp_2;
    u32 x;
    u32 y;
    u32 velocityX;
    u32 velocityY;
    u32 scrollVelocityX;
    u32 scrollVelocityY;
    MouseButtonState buttons;
};
static_assert(sizeof(MouseEntry) == 0x30, "HID mouse entry structure has incorrect size");

struct Mouse {
    MouseHeader header;
    std::array<MouseEntry, 17> entries;
    std::array<u8, 0xB0> padding;
};
static_assert(sizeof(Mouse) == 0x400, "HID mouse structure has incorrect size");

// End Mouse

// Begin Keyboard

struct KeyboardHeader {
    u64 timestampTicks;
    u64 numEntries;
    u64 latestEntry;
    u64 maxEntryIndex;
};
static_assert(sizeof(KeyboardHeader) == 0x20,
              "HID keyboard header structure has incorrect size");

struct KeyboardModifierKeyState {
    union {
        u64 hex{};

        // Buttons
        BitField<0, 1, u64> lctrl;
        BitField<1, 1, u64> lshift;
        BitField<2, 1, u64> lalt;
        BitField<3, 1, u64> lmeta;
        BitField<4, 1, u64> rctrl;
        BitField<5, 1, u64> rshift;
        BitField<6, 1, u64> ralt;
        BitField<7, 1, u64> rmeta;
        BitField<8, 1, u64> capslock;
        BitField<9, 1, u64> scrolllock;
        BitField<10, 1, u64> numlock;
    };
};

struct KeyboardEntry {
    u64 timestamp;
    u64 timestamp_2;
    KeyboardModifierKeyState modifier;
    u32 keys[8];
};
static_assert(sizeof(KeyboardEntry) == 0x38, "HID keyboard entry structure has incorrect size");

struct Keyboard {
    KeyboardHeader header;
    std::array<KeyboardEntry, 17> entries;
    std::array<u8, 0x28> padding;
};
static_assert(sizeof(Keyboard) == 0x400, "HID keyboard structure has incorrect size");

// End Keyboard

// Begin Controller

struct ControllerMAC {
    u64 timestamp;
    std::array<u8, 0x8> mac;
    u64 unk;
    u64 timestamp_2;
};
static_assert(sizeof(ControllerMAC) == 0x20, "HID controller MAC structure has incorrect size");

struct ControllerHeader {
    u32 type;
    u32 isHalf;
    u32 singleColorsDescriptor;
    u32 singleColorBody;
    u32 singleColorButtons;
    u32 splitColorsDescriptor;
    u32 leftColorBody;
    u32 leftColorButtons;
    u32 rightColorBody;
    u32 rightColorbuttons;
};
static_assert(sizeof(ControllerHeader) == 0x28,
              "HID controller header structure has incorrect size");

struct ControllerLayoutHeader {
    u64 timestampTicks;
    u64 numEntries;
    u64 latestEntry;
    u64 maxEntryIndex;
};
static_assert(sizeof(ControllerLayoutHeader) == 0x20,
              "HID controller layout header structure has incorrect size");

struct ControllerPadState {
    union {
        u64 hex{};

        // Buttons
        BitField<0, 1, u64> a;
        BitField<1, 1, u64> b;
        BitField<2, 1, u64> x;
        BitField<3, 1, u64> y;
        BitField<4, 1, u64> lstick;
        BitField<5, 1, u64> rstick;
        BitField<6, 1, u64> l;
        BitField<7, 1, u64> r;
        BitField<8, 1, u64> zl;
        BitField<9, 1, u64> zr;
        BitField<10, 1, u64> plus;
        BitField<11, 1, u64> minus;

        // D-pad buttons
        BitField<12, 1, u64> dleft;
        BitField<13, 1, u64> dup;
        BitField<14, 1, u64> dright;
        BitField<15, 1, u64> ddown;

        // Left stick directions
        BitField<16, 1, u64> lstick_left;
        BitField<17, 1, u64> lstick_up;
        BitField<18, 1, u64> lstick_right;
        BitField<19, 1, u64> lstick_down;

        // Right stick directions
        BitField<20, 1, u64> rstick_left;
        BitField<21, 1, u64> rstick_up;
        BitField<22, 1, u64> rstick_right;
        BitField<23, 1, u64> rstick_down;

        BitField<24, 1, u64> sl;
        BitField<25, 1, u64> sr;
    };
};

struct ControllerInputEntry {
    u64 timestamp;
    u64 timestamp_2;
    ControllerPadState buttons;
    u32 joystickLeftX;
    u32 joystickLeftY;
    u32 joystickRightX;
    u32 joystickRightY;
    u64 connectionState;
};
static_assert(sizeof(ControllerInputEntry) == 0x30,
              "HID controller input entry structure has incorrect size");

struct ControllerLayout {
    ControllerLayoutHeader header;
    std::array<ControllerInputEntry, 17> entries;
};
static_assert(sizeof(ControllerLayout) == 0x350,
              "HID controller layout structure has incorrect size");

struct Controller {
    ControllerHeader header;
    std::array<ControllerLayout, 7> layouts;
    std::array<u8, 0x2a70> unk_1;
    ControllerMAC macLeft;
    ControllerMAC macRight;
    std::array<u8, 0xdf8> unk_2;
};
static_assert(sizeof(Controller) == 0x5000, "HID controller structure has incorrect size");

// End Controller

struct SharedMemory {
    std::array<u8, 0x400> header;
    TouchScreen touchscreen;
    Mouse mouse;
    Keyboard keyboard;
    std::array<u8, 0x400> unkSection1;
    std::array<u8, 0x400> unkSection2;
    std::array<u8, 0x400> unkSection3;
    std::array<u8, 0x400> unkSection4;
    std::array<u8, 0x200> unkSection5;
    std::array<u8, 0x200> unkSection6;
    std::array<u8, 0x200> unkSection7;
    std::array<u8, 0x800> unkSection8;
    std::array<u8, 0x4000> controllerSerials;
    std::array<Controller, 10> controllers;
    std::array<u8, 0x4600> unkSection9;
};
static_assert(sizeof(SharedMemory) == 0x40000, "HID Shared Memory structure has incorrect size");

/// Reload input devices. Used when input configuration changed
void ReloadInputDevices();

/// Registers all HID services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace HID
} // namespace Service
