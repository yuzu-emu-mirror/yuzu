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

        LOG_CRITICAL(Render_OpenGL, "Unimplemented vertex size=%s", attrib.SizeString().c_str());
        UNREACHABLE();
        return {};
    }

    case Maxwell::VertexAttribute::Type::Float:
        return GL_FLOAT;
    }

    LOG_CRITICAL(Render_OpenGL, "Unimplemented vertex type=%s", attrib.TypeString().c_str());
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
    LOG_CRITICAL(Render_OpenGL, "Unimplemented primitive topology=%d", topology);
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
    LOG_CRITICAL(Render_OpenGL, "Unimplemented texture filter mode=%u",
                 static_cast<u32>(filter_mode));
    UNREACHABLE();
    return {};
}

inline GLenum WrapMode(Tegra::Texture::WrapMode wrap_mode) {
    switch (wrap_mode) {
    case Tegra::Texture::WrapMode::Wrap:
        return GL_REPEAT;
    case Tegra::Texture::WrapMode::ClampToEdge:
        return GL_CLAMP_TO_EDGE;
    case Tegra::Texture::WrapMode::ClampOGL:
        // TODO(Subv): GL_CLAMP was removed as of OpenGL 3.1, to implement GL_CLAMP, we can use
        // GL_CLAMP_TO_BORDER to get the border color of the texture, and then sample the edge to
        // manually mix them. However the shader part of this is not yet implemented.
        return GL_CLAMP_TO_BORDER;
    }
    LOG_CRITICAL(Render_OpenGL, "Unimplemented texture wrap mode=%u", static_cast<u32>(wrap_mode));
    UNREACHABLE();
    return {};
}

} // namespace MaxwellToGL
