// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstring>
#include <string>
#include <type_traits>
#include "common/hash.h"

namespace GLShader {

enum Attributes {
    ATTRIBUTE_POSITION,
    ATTRIBUTE_COLOR,
    ATTRIBUTE_TEXCOORD0,
    ATTRIBUTE_TEXCOORD1,
    ATTRIBUTE_TEXCOORD2,
    ATTRIBUTE_TEXCOORD0_W,
    ATTRIBUTE_NORMQUAT,
    ATTRIBUTE_VIEW,
};

struct MaxwellShaderConfigCommon {
    explicit MaxwellShaderConfigCommon(){};
};

struct MaxwellVSConfig : MaxwellShaderConfigCommon {
    explicit MaxwellVSConfig() : MaxwellShaderConfigCommon() {}

    bool operator==(const MaxwellVSConfig& o) const {
        return std::memcmp(this, &o, sizeof(MaxwellVSConfig)) == 0;
    };
};

struct MaxwellFSConfig : MaxwellShaderConfigCommon {
    explicit MaxwellFSConfig() : MaxwellShaderConfigCommon() {}

    bool operator==(const MaxwellFSConfig& o) const {
        return std::memcmp(this, &o, sizeof(MaxwellFSConfig)) == 0;
    };
};

std::string GenerateVertexShader(const MaxwellVSConfig& config);
std::string GenerateFragmentShader(const MaxwellFSConfig& config);

} // namespace GLShader

namespace std {

template <>
struct hash<GLShader::MaxwellVSConfig> {
    size_t operator()(const GLShader::MaxwellVSConfig& k) const {
        return Common::ComputeHash64(&k, sizeof(GLShader::MaxwellVSConfig));
    }
};

template <>
struct hash<GLShader::MaxwellFSConfig> {
    size_t operator()(const GLShader::MaxwellFSConfig& k) const {
        return Common::ComputeHash64(&k, sizeof(GLShader::MaxwellFSConfig));
    }
};

} // namespace std
