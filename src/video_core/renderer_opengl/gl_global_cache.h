// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <map>
#include <memory>

#include "common/common_types.h"
#include "video_core/rasterizer_cache.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/renderer_opengl/gl_shader_gen.h"

namespace OpenGL {

class CachedGlobalRegion;
using GlobalRegion = std::shared_ptr<CachedGlobalRegion>;
using Maxwell = Tegra::Engines::Maxwell3D::Regs;

class CachedGlobalRegion final : public RasterizerCacheObject {
public:
    CachedGlobalRegion(VAddr addr, u32 size);

    /// Gets the address of the shader in guest memory, required for cache management
    VAddr GetAddr() const {
        return addr;
    }

    /// Gets the size of the shader in guest memory, required for cache management
    std::size_t GetSizeInBytes() const {
        return size;
    }

    /// Gets the GL program handle for the buffer
    GLuint GetBufferHandle() const {
        return buffer.handle;
    }

    // We do not have to flush this cache as things in it are never modified by us.
    void Flush() override {}

private:
    VAddr addr;
    u32 size;

    OGLBuffer buffer;
};

class GlobalRegionCacheOpenGL final : public RasterizerCache<GlobalRegion> {
public:
    /// Gets the current specified shader stage program
    GlobalRegion GetGlobalRegion(Tegra::Engines::Maxwell3D::GlobalMemoryDescriptor descriptor,
                                 Tegra::Engines::Maxwell3D::Regs::ShaderStage stage);
};

} // namespace OpenGL
