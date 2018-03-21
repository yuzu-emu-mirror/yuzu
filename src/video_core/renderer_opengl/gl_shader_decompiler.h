// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <functional>
#include <string>
#include "common/common_types.h"

namespace Maxwell3D {
namespace Shader {
namespace Decompiler {

constexpr size_t MAX_PROGRAM_CODE_LENGTH{0x100000};
constexpr size_t MAX_SWIZZLE_DATA_LENGTH{0x100000};

std::string DecompileProgram(const std::array<u32, MAX_PROGRAM_CODE_LENGTH>& program_code,
                             const std::array<u32, MAX_SWIZZLE_DATA_LENGTH>& swizzle_data,
                             u32 main_offset,
                             const std::function<std::string(u32)>& inputreg_getter,
                             const std::function<std::string(u32)>& outputreg_getter,
                             bool sanitize_mul, const std::string& emit_cb = "",
                             const std::string& setemit_cb = "");

} // namespace Decompiler
} // namespace Shader
} // namespace Maxwell3D
