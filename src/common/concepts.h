// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <concepts>

// Check if type is like an STL container
template <typename T>
concept IsSTLContainer = requires(T t) {
    typename T::value_type;
    // TODO(ogniK): Replace below is std::same_as<void> when MSVC supports it.
    t.begin();
    t.end();
    t.data();
    t.size();
};
