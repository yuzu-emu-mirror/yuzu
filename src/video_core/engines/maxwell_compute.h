// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "common/common_types.h"

namespace Tegra {
namespace Engines {

class MaxwellCompute final {
public:
    MaxwellCompute() = default;
    ~MaxwellCompute() = default;

    /// Write the value to the register identified by method.
    void WriteReg(u32 method, u32 value);

    /**
     * Handles a method call to this engine.
     * @param method Method to call
     * @param parameters Arguments to the method call
     */
    void CallMethod(u32 method, const std::vector<u32>& parameters);
};

} // namespace Engines
} // namespace Tegra
