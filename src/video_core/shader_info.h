// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>

#include "common/common_types.h"

namespace VideoCore {

enum class ShaderStage {
    Vertex,
    TesselationControl,
    TesselationEval,
    Geometry,
    Fragment,
    Compute,
    Invalid,
};

struct ShaderInfo {
    u64 addr;
    ShaderStage stage;
    std::string code;
};

} // namespace VideoCore
