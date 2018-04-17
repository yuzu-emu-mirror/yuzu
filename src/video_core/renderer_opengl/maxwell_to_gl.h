// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <glad/glad.h>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "video_core/engines/maxwell_3d.h"

using GLvec2 = std::array<GLfloat, 2>;
using GLvec3 = std::array<GLfloat, 3>;
using GLvec4 = std::array<GLfloat, 4>;

using GLuvec2 = std::array<GLuint, 2>;
using GLuvec3 = std::array<GLuint, 3>;
using GLuvec4 = std::array<GLuint, 4>;

namespace MaxwellToGL {

using Maxwell = Tegra::Engines::Maxwell3D::Regs;

inline GLenum VertexType(Maxwell::VertexAttribute attrib) {
    switch (attrib.type) {
    case Maxwell::VertexAttribute::Type::UnsignedNorm: {

        switch (attrib.size) {
        case Maxwell::VertexAttribute::Size::Size_8_8_8_8:
            return GL_UNSIGNED_BYTE;
        }

        NGLOG_CRITICAL(Render_OpenGL, "Unimplemented vertex size={}", attrib.SizeString());
        UNREACHABLE();
        return {};
    }

    case Maxwell::VertexAttribute::Type::Float:
        return GL_FLOAT;
    }

    NGLOG_CRITICAL(Render_OpenGL, "Unimplemented vertex type={}", attrib.TypeString());
    UNREACHABLE();
    return {};
}

inline GLenum IndexFormat(Maxwell::IndexFormat index_format) {
    switch (index_format) {
    case Maxwell::IndexFormat::UnsignedByte:
        return GL_UNSIGNED_BYTE;
    case Maxwell::IndexFormat::UnsignedShort:
        return GL_UNSIGNED_SHORT;
    case Maxwell::IndexFormat::UnsignedInt:
        return GL_UNSIGNED_INT;
    }
    NGLOG_CRITICAL(Render_OpenGL, "Unimplemented index_format={}", static_cast<u32>(index_format));
    UNREACHABLE();
    return {};
}

inline GLenum PrimitiveTopology(Maxwell::PrimitiveTopology topology) {
    switch (topology) {
    case Maxwell::PrimitiveTopology::Triangles:
        return GL_TRIANGLES;
    case Maxwell::PrimitiveTopology::TriangleStrip:
        return GL_TRIANGLE_STRIP;
    }
    NGLOG_CRITICAL(Render_OpenGL, "Unimplemented topology={}", static_cast<u32>(topology));
    UNREACHABLE();
    return {};
}

inline GLenum TextureFilterMode(Tegra::Texture::TextureFilter filter_mode) {
    switch (filter_mode) {
    case Tegra::Texture::TextureFilter::Linear:
        return GL_LINEAR;
    case Tegra::Texture::TextureFilter::Nearest:
        return GL_NEAREST;
    }
    NGLOG_CRITICAL(Render_OpenGL, "Unimplemented texture filter mode={}",
                   static_cast<u32>(filter_mode));
    UNREACHABLE();
    return {};
}

inline GLenum WrapMode(Tegra::Texture::WrapMode wrap_mode) {
    switch (wrap_mode) {
    case Tegra::Texture::WrapMode::ClampToEdge:
        return GL_CLAMP_TO_EDGE;
    }
    NGLOG_CRITICAL(Render_OpenGL, "Unimplemented texture wrap mode={}",
                   static_cast<u32>(wrap_mode));
    UNREACHABLE();
    return {};
}

} // namespace MaxwellToGL
