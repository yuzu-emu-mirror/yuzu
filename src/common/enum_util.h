// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <type_traits>

namespace Common {

template <typename Enum>
    requires std::is_enum<Enum>::value
constexpr typename std::underlying_type<Enum>::type ToUnderlying(Enum e) {
    return static_cast<typename std::underlying_type<Enum>::type>(e);
}

template <typename Enum>
    requires std::is_enum<Enum>::value
constexpr Enum FromUnderlying(typename std::underlying_type<Enum>::type v) {
    return static_cast<Enum>(v);
}

} // namespace Common
