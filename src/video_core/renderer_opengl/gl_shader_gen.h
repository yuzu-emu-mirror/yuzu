// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <string>
#include <type_traits>
#include "common/common_types.h"
#include "common/hash.h"

namespace GLShader {

constexpr size_t MAX_PROGRAM_CODE_LENGTH{0x1000};

using ProgramCode = std::array<u64, MAX_PROGRAM_CODE_LENGTH>;

struct ShaderSetup {
    ShaderSetup(ProgramCode&& program_code) : program_code(std::move(program_code)) {}

    ProgramCode program_code;
    bool program_code_hash_dirty = true;

    u64 GetProgramCodeHash() {
        if (program_code_hash_dirty) {
            program_code_hash = Common::ComputeHash64(&program_code, sizeof(program_code));
            program_code_hash_dirty = false;
        }
        return program_code_hash;
    }

private:
    u64 program_code_hash{};
};

struct MaxwellShaderConfigCommon {
    void Init(ShaderSetup& setup) {
        program_hash = setup.GetProgramCodeHash();
    }

    u64 program_hash;
};

struct MaxwellVSConfig : Common::HashableStruct<MaxwellShaderConfigCommon> {
    explicit MaxwellVSConfig(ShaderSetup& setup) {
        state.Init(setup);
    }
};

struct MaxwellFSConfig : Common::HashableStruct<MaxwellShaderConfigCommon> {
    explicit MaxwellFSConfig(ShaderSetup& setup) {
        state.Init(setup);
    }
};

/**
 * Generates the GLSL vertex shader program source code for the given VS program
 * @returns String of the shader source code
 */
std::string GenerateVertexShader(const ShaderSetup& setup, const MaxwellVSConfig& config);

/**
 * Generates the GLSL fragment shader program source code for the given FS program
 * @returns String of the shader source code
 */
std::string GenerateFragmentShader(const ShaderSetup& setup, const MaxwellFSConfig& config);

} // namespace GLShader

namespace std {

template <>
struct hash<GLShader::MaxwellVSConfig> {
    size_t operator()(const GLShader::MaxwellVSConfig& k) const {
        return k.Hash();
    }
};

template <>
struct hash<GLShader::MaxwellFSConfig> {
    size_t operator()(const GLShader::MaxwellFSConfig& k) const {
        return k.Hash();
    }
};

} // namespace std
