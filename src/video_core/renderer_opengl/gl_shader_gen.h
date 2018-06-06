// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "common/common_types.h"
#include "common/hash.h"

namespace GLShader {

constexpr size_t MAX_PROGRAM_CODE_LENGTH{0x1000};

using ProgramCode = std::array<u64, MAX_PROGRAM_CODE_LENGTH>;

class ConstBufferEntry {
    using Maxwell = Tegra::Engines::Maxwell3D::Regs;

public:
    void MarkAsUsed(u64 index, u64 offset, Maxwell::ShaderStage stage) {
        is_used = true;
        this->index = static_cast<unsigned>(index);
        this->stage = stage;
        max_offset = std::max(max_offset, static_cast<unsigned>(offset));
    }

    void MarkAsUsedIndirect(u64 index, Maxwell::ShaderStage stage) {
        is_used = true;
        is_indirect = true;
        this->index = static_cast<unsigned>(index);
        this->stage = stage;
    }

    bool IsUsed() const {
        return is_used;
    }

    bool IsIndirect() const {
        return is_indirect;
    }

    unsigned GetIndex() const {
        return index;
    }

    unsigned GetSize() const {
        return max_offset + 1;
    }

    std::string GetName() const {
        return BufferBaseNames[static_cast<size_t>(stage)] + std::to_string(index);
    }

private:
    static constexpr std::array<const char*, Maxwell::MaxShaderStage> BufferBaseNames = {
        "buffer_vs_c", "buffer_tessc_c", "buffer_tesse_c", "buffer_gs_c", "buffer_fs_c",
    };

    bool is_used{};
    bool is_indirect{};
    unsigned index{};
    unsigned max_offset{};
    Maxwell::ShaderStage stage;
};

struct ShaderEntries {
    std::vector<ConstBufferEntry> const_buffer_entries;
};

using ProgramResult = std::pair<std::string, ShaderEntries>;

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
ProgramResult GenerateVertexShader(const ShaderSetup& setup, const MaxwellVSConfig& config);

/**
 * Generates the GLSL fragment shader program source code for the given FS program
 * @returns String of the shader source code
 */
ProgramResult GenerateFragmentShader(const ShaderSetup& setup, const MaxwellFSConfig& config);

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
