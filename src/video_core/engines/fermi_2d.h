// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

namespace Tegra {
namespace Engines {

class Fermi2D final {
public:
    Fermi2D() = default;
    ~Fermi2D() = default;

    /// Write the value to the register identified by method.
    void WriteReg(u32 method, u32 value);
};

} // namespace Engines
} // namespace Tegra
