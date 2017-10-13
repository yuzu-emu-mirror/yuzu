// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

namespace Service {
namespace HID {

/// Initialize HID service
void Init();

/// Shutdown HID service
void Shutdown();

/// Reload input devices. Used when input configuration changed
void ReloadInputDevices();

} // namespace HID
} // namespace Service
