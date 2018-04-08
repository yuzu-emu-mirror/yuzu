// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <functional>
#include <string>
#include <boost/optional.hpp>
#include "common/common_types.h"
#include "video_core/renderer_opengl/gl_shader_gen.h"

namespace GLShader {
namespace Decompiler {

std::string GetCommonDeclarations();

boost::optional<std::string> DecompileProgram(const ProgramCode& program_code, u32 main_offset);

} // namespace Decompiler
} // namespace GLShader
