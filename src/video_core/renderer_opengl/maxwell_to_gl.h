// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <glad/glad.h>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "video_core/engines/maxwell_3d.h"

namespace MaxwellToGL {

using Maxwell = Tegra::Engines::Maxwell3D::Regs;

inline GLenum VertexType(Maxwell::VertexAttribute attrib) {
    switch (attrib.type) {
    case Maxwell::VertexAttribute::Type::UnsignedNorm: {

        switch (attrib.size) {
        case Maxwell::VertexAttribute::Size::Size_8_8_8_8:
            return GL_UNSIGNED_BYTE;
        }

        LOG_CRITICAL(Render_OpenGL, "Unimplemented vertex size=%s", attrib.SizeString());
        UNREACHABLE();
        return {};
    }

    case Maxwell::VertexAttribute::Type::Float:
        return GL_FLOAT;
    }

    LOG_CRITICAL(Render_OpenGL, "Unimplemented vertex type=%s", attrib.TypeString());
    UNREACHABLE();
    return {};
}

} // namespace MaxwellToGL
