// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <string>
#include <queue>
#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/renderer_opengl/gl_shader_decompiler.h"

namespace Maxwell3D {
namespace Shader {
namespace Decompiler {

constexpr u32 PROGRAM_END = MAX_PROGRAM_CODE_LENGTH;

class Impl {
public:
    Impl(const std::array<u32, MAX_PROGRAM_CODE_LENGTH>& program_code,
         const std::array<u32, MAX_SWIZZLE_DATA_LENGTH>& swizzle_data, u32 main_offset,
         const std::function<std::string(u32)>& inputreg_getter,
         const std::function<std::string(u32)>& outputreg_getter, bool sanitize_mul,
         const std::string& emit_cb, const std::string& setemit_cb)
        : program_code(program_code), swizzle_data(swizzle_data), main_offset(main_offset),
          inputreg_getter(inputreg_getter), outputreg_getter(outputreg_getter),
          sanitize_mul(sanitize_mul), emit_cb(emit_cb), setemit_cb(setemit_cb) {}

    std::string Decompile() {
        UNIMPLEMENTED();
        return {};
    }

private:
    const std::array<u32, MAX_PROGRAM_CODE_LENGTH>& program_code;
    const std::array<u32, MAX_SWIZZLE_DATA_LENGTH>& swizzle_data;
    u32 main_offset;
    const std::function<std::string(u32)>& inputreg_getter;
    const std::function<std::string(u32)>& outputreg_getter;
    bool sanitize_mul;
    const std::string& emit_cb;
    const std::string& setemit_cb;
};

std::string DecompileProgram(const std::array<u32, MAX_PROGRAM_CODE_LENGTH>& program_code,
                             const std::array<u32, MAX_SWIZZLE_DATA_LENGTH>& swizzle_data,
                             u32 main_offset,
                             const std::function<std::string(u32)>& inputreg_getter,
                             const std::function<std::string(u32)>& outputreg_getter,
                             bool sanitize_mul, const std::string& emit_cb,
                             const std::string& setemit_cb) {
    Impl impl(program_code, swizzle_data, main_offset, inputreg_getter, outputreg_getter,
              sanitize_mul, emit_cb, setemit_cb);
    return impl.Decompile();
}

} // namespace Decompiler
} // namespace Shader
} // namespace Maxwell3D
