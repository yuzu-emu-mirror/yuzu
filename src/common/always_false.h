// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Common {

template <typename T>
struct always_false {
    static constexpr bool value = false;
};

template <typename T>
inline constexpr bool always_false_v = false;

} // namespace Common
