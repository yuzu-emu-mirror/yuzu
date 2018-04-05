// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <functional>
#include <string>
#include <boost/optional.hpp>
#include "common/common_types.h"

namespace Tegra {
namespace Shader {
namespace Decompiler {

constexpr size_t MAX_PROGRAM_CODE_LENGTH{0x100};
constexpr size_t MAX_SWIZZLE_DATA_LENGTH{0x100};

using ProgramCode = std::array<u64, MAX_PROGRAM_CODE_LENGTH>;

boost::optional<std::string> DecompileProgram(const ProgramCode& program_code, u32 main_offset);

} // namespace Decompiler
} // namespace Shader
} // namespace Tegra
