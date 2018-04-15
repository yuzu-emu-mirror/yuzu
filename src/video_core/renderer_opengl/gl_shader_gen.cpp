// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/renderer_opengl/gl_shader_decompiler.h"
#include "video_core/renderer_opengl/gl_shader_gen.h"

namespace GLShader {

using Tegra::Engines::Maxwell3D;

static constexpr u32 PROGRAM_OFFSET{10};

ProgramResult GenerateVertexShader(const ShaderSetup& setup, const MaxwellVSConfig& config) {
    std::string out = "#version 430 core\n";
    out += "#extension GL_ARB_separate_shader_objects : enable\n\n";
    out += Decompiler::GetCommonDeclarations();

    ProgramResult program = Decompiler::DecompileProgram(setup.program_code, PROGRAM_OFFSET,
                                                         Maxwell3D::Regs::ShaderStage::Vertex)
                                .get_value_or({});
    out += R"(

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    exec_shader();
}

)";
    out += program.first;
    return {out, program.second};
}

ProgramResult GenerateFragmentShader(const ShaderSetup& setup, const MaxwellFSConfig& config) {
    std::string out = "#version 430 core\n";
    out += "#extension GL_ARB_separate_shader_objects : enable\n\n";
    out += Decompiler::GetCommonDeclarations();

    ProgramResult program = Decompiler::DecompileProgram(setup.program_code, PROGRAM_OFFSET,
                                                         Maxwell3D::Regs::ShaderStage::Fragment)
                                .get_value_or({});
    out += R"(

out vec4 color;

uniform sampler2D tex[32];

void main() {
    exec_shader();
}

)";
    out += program.first;
    return {out, program.second};
}

} // namespace GLShader
