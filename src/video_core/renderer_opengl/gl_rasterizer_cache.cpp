// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <glad/glad.h>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/microprofile.h"
#include "common/scope_exit.h"
#include "core/core.h"
#include "core/hle/kernel/process.h"
#include "core/memory.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/renderer_opengl/gl_rasterizer_cache.h"
#include "video_core/textures/astc.h"
#include "video_core/textures/decoders.h"
#include "video_core/utils.h"

using SurfaceType = SurfaceParams::SurfaceType;
using PixelFormat = SurfaceParams::PixelFormat;
using ComponentType = SurfaceParams::ComponentType;

struct FormatTuple {
    GLint internal_format;
    GLenum format;
    GLenum type;
    bool compressed;
};

static constexpr std::array<FormatTuple, SurfaceParams::MaxPixelFormat> tex_format_tuples = {{
    {GL_RGBA8, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, false},                    // ABGR8
    {GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV, false},                       // B5G6R5
    {GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, false},              // A2B10G10R10
    {GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, false},                // A1B5G5R5
    {GL_R8, GL_RED, GL_UNSIGNED_BYTE, false},                                   // R8
    {GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, false},                                // RGBA16F
    {GL_R11F_G11F_B10F, GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV, false},        // R11FG11FB10F
    {GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_RGB, GL_UNSIGNED_INT_8_8_8_8, true},   // DXT1
    {GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, true}, // DXT23
    {GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, true}, // DXT45
    {GL_COMPRESSED_RED_RGTC1, GL_RED, GL_UNSIGNED_INT_8_8_8_8, true},           // DXN1
    {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, false},                               // ASTC_2D_4X4
}};

static const FormatTuple& GetFormatTuple(PixelFormat pixel_format, ComponentType component_type) {
    const SurfaceType type = SurfaceParams::GetFormatType(pixel_format);
    if (type == SurfaceType::ColorTexture) {
        ASSERT(static_cast<size_t>(pixel_format) < tex_format_tuples.size());
        // For now only UNORM components are supported, or either R11FG11FB10F or RGBA16F which are
        // type FLOAT
        ASSERT(component_type == ComponentType::UNorm || pixel_format == PixelFormat::RGBA16F ||
               pixel_format == PixelFormat::R11FG11FB10F);
        return tex_format_tuples[static_cast<unsigned int>(pixel_format)];
    } else if (type == SurfaceType::Depth || type == SurfaceType::DepthStencil) {
        // TODO(Subv): Implement depth formats
        ASSERT_MSG(false, "Unimplemented");
    }

    UNREACHABLE();
    return {};
}

VAddr SurfaceParams::GetCpuAddr() const {
    const auto& gpu = Core::System::GetInstance().GPU();
    return *gpu.memory_manager->GpuToCpuAddress(addr);
}

static bool IsPixelFormatASTC(PixelFormat format) {
    switch (format) {
    case PixelFormat::ASTC_2D_4X4:
        return true;
    default:
        return false;
    }
}

static void ConvertASTCToRGBA8(std::vector<u8>& data, PixelFormat format, u32 width, u32 height) {
    u32 block_width{};
    u32 block_height{};

    switch (format) {
    case PixelFormat::ASTC_2D_4X4:
        block_width = 4;
        block_height = 4;
        break;
    default:
        NGLOG_CRITICAL(HW_GPU, "Unhandled format: {}", static_cast<u32>(format));
        UNREACHABLE();
    }

    data = Tegra::Texture::ASTC::Decompress(data, width, height, block_width, block_height);
}

template <bool morton_to_gl, PixelFormat format>
void MortonCopy(u32 stride, u32 block_height, u32 height, u8* gl_buffer, Tegra::GPUVAddr addr) {
    constexpr u32 bytes_per_pixel = SurfaceParams::GetFormatBpp(format) / CHAR_BIT;
    constexpr u32 gl_bytes_per_pixel = CachedSurface::GetGLBytesPerPixel(format);
    const auto& gpu = Core::System::GetInstance().GPU();

    if (morton_to_gl) {
        auto data = Tegra::Texture::UnswizzleTexture(
            *gpu.memory_manager->GpuToCpuAddress(addr),
            SurfaceParams::TextureFormatFromPixelFormat(format), stride, height, block_height);

        if (IsPixelFormatASTC(format)) {
            // ASTC formats are converted to RGBA8 in software, as most PC GPUs do not support this
            ConvertASTCToRGBA8(data, format, stride, height);
        }

        std::memcpy(gl_buffer, data.data(), data.size());
    } else {
        // TODO(bunnei): Assumes the default rendering GOB size of 16 (128 lines). We should check
        // the configuration for this and perform more generic un/swizzle
        NGLOG_WARNING(Render_OpenGL, "need to use correct swizzle/GOB parameters!");
        VideoCore::MortonCopyPixels128(
            stride, height, bytes_per_pixel, gl_bytes_per_pixel,
            Memory::GetPointer(*gpu.memory_manager->GpuToCpuAddress(addr)), gl_buffer,
            morton_to_gl);
    }
}

static constexpr std::array<void (*)(u32, u32, u32, u8*, Tegra::GPUVAddr),
                            SurfaceParams::MaxPixelFormat>
    morton_to_gl_fns = {
        MortonCopy<true, PixelFormat::ABGR8>,        MortonCopy<true, PixelFormat::B5G6R5>,
        MortonCopy<true, PixelFormat::A2B10G10R10>,  MortonCopy<true, PixelFormat::A1B5G5R5>,
        MortonCopy<true, PixelFormat::R8>,           MortonCopy<true, PixelFormat::RGBA16F>,
        MortonCopy<true, PixelFormat::R11FG11FB10F>, MortonCopy<true, PixelFormat::DXT1>,
        MortonCopy<true, PixelFormat::DXT23>,        MortonCopy<true, PixelFormat::DXT45>,
        MortonCopy<true, PixelFormat::DXN1>,         MortonCopy<true, PixelFormat::ASTC_2D_4X4>,
};

static constexpr std::array<void (*)(u32, u32, u32, u8*, Tegra::GPUVAddr),
                            SurfaceParams::MaxPixelFormat>
    gl_to_morton_fns = {
        MortonCopy<false, PixelFormat::ABGR8>,
        MortonCopy<false, PixelFormat::B5G6R5>,
        MortonCopy<false, PixelFormat::A2B10G10R10>,
        MortonCopy<false, PixelFormat::A1B5G5R5>,
        MortonCopy<false, PixelFormat::R8>,
        MortonCopy<false, PixelFormat::RGBA16F>,
        MortonCopy<false, PixelFormat::R11FG11FB10F>,
        // TODO(Subv): Swizzling the DXT1/DXT23/DXT45/DXN1 formats is not yet supported
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        MortonCopy<false, PixelFormat::ABGR8>,
};

// Allocate an uninitialized texture of appropriate size and format for the surface
static void AllocateSurfaceTexture(GLuint texture, const FormatTuple& format_tuple, u32 width,
                                   u32 height) {
    OpenGLState cur_state = OpenGLState::GetCurState();

    // Keep track of previous texture bindings
    GLuint old_tex = cur_state.texture_units[0].texture_2d;
    cur_state.texture_units[0].texture_2d = texture;
    cur_state.Apply();
    glActiveTexture(GL_TEXTURE0);

    if (!format_tuple.compressed) {
        // Only pre-create the texture for non-compressed textures.
        glTexImage2D(GL_TEXTURE_2D, 0, format_tuple.internal_format, width, height, 0,
                     format_tuple.format, format_tuple.type, nullptr);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Restore previous texture bindings
    cur_state.texture_units[0].texture_2d = old_tex;
    cur_state.Apply();
}

CachedSurface::CachedSurface(const SurfaceParams& params) : params(params), gl_buffer_size(0) {
    texture.Create();
    AllocateSurfaceTexture(texture.handle,
                           GetFormatTuple(params.pixel_format, params.component_type), params.width,
                           params.height);
}

MICROPROFILE_DEFINE(OpenGL_SurfaceLoad, "OpenGL", "Surface Load", MP_RGB(128, 64, 192));
void CachedSurface::LoadGLBuffer() {
    ASSERT(params.type != SurfaceType::Fill);

    u8* const texture_src_data = Memory::GetPointer(params.GetCpuAddr());

    ASSERT(texture_src_data);

    if (!gl_buffer) {
        gl_buffer_size = params.width * params.height * GetGLBytesPerPixel(params.pixel_format);
        gl_buffer.reset(new u8[gl_buffer_size]);
    }

    MICROPROFILE_SCOPE(OpenGL_SurfaceLoad);

    if (!params.is_tiled) {
        const u32 bytes_per_pixel{params.GetFormatBpp() >> 3};

        std::memcpy(&gl_buffer[0], texture_src_data,
                    bytes_per_pixel * params.width * params.height);
    } else {
        morton_to_gl_fns[static_cast<size_t>(params.pixel_format)](
            params.width, params.block_height, params.height, &gl_buffer[0], params.addr);
    }
}

MICROPROFILE_DEFINE(OpenGL_SurfaceFlush, "OpenGL", "Surface Flush", MP_RGB(128, 192, 64));
void CachedSurface::FlushGLBuffer() {
    u8* const dst_buffer = Memory::GetPointer(params.GetCpuAddr());

    ASSERT(dst_buffer);
    ASSERT(gl_buffer_size ==
           params.width * params.height * GetGLBytesPerPixel(params.pixel_format));

    MICROPROFILE_SCOPE(OpenGL_SurfaceFlush);

    if (!params.is_tiled) {
        std::memcpy(dst_buffer, &gl_buffer[0], params.SizeInBytes());
    } else {
        gl_to_morton_fns[static_cast<size_t>(params.pixel_format)](
            params.width, params.block_height, params.height, &gl_buffer[0], params.addr);
    }
}

MICROPROFILE_DEFINE(OpenGL_TextureUL, "OpenGL", "Texture Upload", MP_RGB(128, 64, 192));
void CachedSurface::UploadGLTexture(GLuint read_fb_handle, GLuint draw_fb_handle) {
    if (params.type == SurfaceType::Fill)
        return;

    MICROPROFILE_SCOPE(OpenGL_TextureUL);

    ASSERT(gl_buffer_size ==
           params.width * params.height * GetGLBytesPerPixel(params.pixel_format));

    const auto& rect{params.GetRect()};

    // Load data from memory to the surface
    GLint x0 = static_cast<GLint>(rect.left);
    GLint y0 = static_cast<GLint>(rect.bottom);
    size_t buffer_offset = (y0 * params.width + x0) * GetGLBytesPerPixel(params.pixel_format);

    const FormatTuple& tuple = GetFormatTuple(params.pixel_format, params.component_type);
    GLuint target_tex = texture.handle;
    OpenGLState cur_state = OpenGLState::GetCurState();

    GLuint old_tex = cur_state.texture_units[0].texture_2d;
    cur_state.texture_units[0].texture_2d = target_tex;
    cur_state.Apply();

    // Ensure no bad interactions with GL_UNPACK_ALIGNMENT
    ASSERT(params.width * GetGLBytesPerPixel(params.pixel_format) % 4 == 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(params.width));

    glActiveTexture(GL_TEXTURE0);
    if (tuple.compressed) {
        glCompressedTexImage2D(
            GL_TEXTURE_2D, 0, tuple.internal_format, static_cast<GLsizei>(params.width),
            static_cast<GLsizei>(params.height), 0, static_cast<GLsizei>(params.SizeInBytes()),
            &gl_buffer[buffer_offset]);
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, x0, y0, static_cast<GLsizei>(rect.GetWidth()),
                        static_cast<GLsizei>(rect.GetHeight()), tuple.format, tuple.type,
                        &gl_buffer[buffer_offset]);
    }

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    cur_state.texture_units[0].texture_2d = old_tex;
    cur_state.Apply();
}

MICROPROFILE_DEFINE(OpenGL_TextureDL, "OpenGL", "Texture Download", MP_RGB(128, 192, 64));
void CachedSurface::DownloadGLTexture(GLuint read_fb_handle, GLuint draw_fb_handle) {
    if (params.type == SurfaceType::Fill)
        return;

    MICROPROFILE_SCOPE(OpenGL_TextureDL);

    if (!gl_buffer) {
        gl_buffer_size = params.width * params.height * GetGLBytesPerPixel(params.pixel_format);
        gl_buffer.reset(new u8[gl_buffer_size]);
    }

    OpenGLState state = OpenGLState::GetCurState();
    OpenGLState prev_state = state;
    SCOPE_EXIT({ prev_state.Apply(); });

    const FormatTuple& tuple = GetFormatTuple(params.pixel_format, params.component_type);

    // Ensure no bad interactions with GL_PACK_ALIGNMENT
    ASSERT(params.width * GetGLBytesPerPixel(params.pixel_format) % 4 == 0);
    glPixelStorei(GL_PACK_ROW_LENGTH, static_cast<GLint>(params.width));

    const auto& rect{params.GetRect()};
    size_t buffer_offset =
        (rect.bottom * params.width + rect.left) * GetGLBytesPerPixel(params.pixel_format);

    state.UnbindTexture(texture.handle);
    state.draw.read_framebuffer = read_fb_handle;
    state.Apply();

    if (params.type == SurfaceType::ColorTexture) {
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               texture.handle, 0);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0,
                               0);
    } else if (params.type == SurfaceType::Depth) {
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                               texture.handle, 0);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    } else {
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                               texture.handle, 0);
    }
    glReadPixels(static_cast<GLint>(rect.left), static_cast<GLint>(rect.bottom),
                 static_cast<GLsizei>(rect.GetWidth()), static_cast<GLsizei>(rect.GetHeight()),
                 tuple.format, tuple.type, &gl_buffer[buffer_offset]);

    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
}

RasterizerCacheOpenGL::RasterizerCacheOpenGL() {
    read_framebuffer.Create();
    draw_framebuffer.Create();
}

Surface RasterizerCacheOpenGL::GetTextureSurface(const Tegra::Texture::FullTextureInfo& config) {
    auto& gpu = Core::System::GetInstance().GPU();

    SurfaceParams params;
    params.addr = config.tic.Address();
    params.is_tiled = config.tic.IsTiled();
    params.pixel_format = SurfaceParams::PixelFormatFromTextureFormat(config.tic.format);
    params.component_type = SurfaceParams::ComponentTypeFromTexture(config.tic.r_type.Value());
    params.type = SurfaceParams::GetFormatType(params.pixel_format);
    params.width = Common::AlignUp(config.tic.Width(), params.GetCompressionFactor());
    params.height = Common::AlignUp(config.tic.Height(), params.GetCompressionFactor());

    if (params.is_tiled) {
        params.block_height = config.tic.BlockHeight();
    }

    // TODO(Subv): Different types per component are not supported.
    ASSERT(config.tic.r_type.Value() == config.tic.g_type.Value() &&
           config.tic.r_type.Value() == config.tic.b_type.Value() &&
           config.tic.r_type.Value() == config.tic.a_type.Value());

    return GetSurface(params);
}

SurfaceSurfaceRect_Tuple RasterizerCacheOpenGL::GetFramebufferSurfaces(
    bool using_color_fb, bool using_depth_fb, const MathUtil::Rectangle<s32>& viewport) {
    const auto& regs = Core::System().GetInstance().GPU().Maxwell3D().regs;
    const auto& config = regs.rt[0];

    // TODO(bunnei): This is hard corded to use just the first render buffer
    NGLOG_WARNING(Render_OpenGL, "hard-coded for render target 0!");

    MathUtil::Rectangle<u32> viewport_clamped{
        static_cast<u32>(std::clamp(viewport.left, 0, static_cast<s32>(config.width))),
        static_cast<u32>(std::clamp(viewport.top, 0, static_cast<s32>(config.height))),
        static_cast<u32>(std::clamp(viewport.right, 0, static_cast<s32>(config.width))),
        static_cast<u32>(std::clamp(viewport.bottom, 0, static_cast<s32>(config.height)))};

    // get color and depth surfaces
    SurfaceParams color_params;
    color_params.is_tiled = true;
    color_params.width = config.width;
    color_params.height = config.height;
    // TODO(Subv): Can framebuffers use a different block height?
    color_params.block_height = Tegra::Texture::TICEntry::DefaultBlockHeight;
    SurfaceParams depth_params = color_params;

    color_params.addr = config.Address();
    color_params.pixel_format = SurfaceParams::PixelFormatFromRenderTargetFormat(config.format);
    color_params.component_type = SurfaceParams::ComponentTypeFromRenderTarget(config.format);
    color_params.type = SurfaceParams::GetFormatType(color_params.pixel_format);

    ASSERT_MSG(!using_depth_fb, "depth buffer is unimplemented");

    MathUtil::Rectangle<u32> color_rect{};
    Surface color_surface;
    if (using_color_fb) {
        color_surface = GetSurface(color_params);
        color_rect = color_surface->GetSurfaceParams().GetRect();
    }

    MathUtil::Rectangle<u32> depth_rect{};
    Surface depth_surface;
    if (using_depth_fb) {
        depth_surface = GetSurface(depth_params);
        depth_rect = depth_surface->GetSurfaceParams().GetRect();
    }

    MathUtil::Rectangle<u32> fb_rect{};
    if (color_surface && depth_surface) {
        fb_rect = color_rect;
        // Color and Depth surfaces must have the same dimensions and offsets
        if (color_rect.bottom != depth_rect.bottom || color_rect.top != depth_rect.top ||
            color_rect.left != depth_rect.left || color_rect.right != depth_rect.right) {
            color_surface = GetSurface(color_params);
            depth_surface = GetSurface(depth_params);
            fb_rect = color_surface->GetSurfaceParams().GetRect();
        }
    } else if (color_surface) {
        fb_rect = color_rect;
    } else if (depth_surface) {
        fb_rect = depth_rect;
    }

    return std::make_tuple(color_surface, depth_surface, fb_rect);
}

void RasterizerCacheOpenGL::LoadSurface(const Surface& surface) {
    surface->LoadGLBuffer();
    surface->UploadGLTexture(read_framebuffer.handle, draw_framebuffer.handle);
}

void RasterizerCacheOpenGL::FlushSurface(const Surface& surface) {
    surface->DownloadGLTexture(read_framebuffer.handle, draw_framebuffer.handle);
    surface->FlushGLBuffer();
}

Surface RasterizerCacheOpenGL::GetSurface(const SurfaceParams& params) {
    if (params.addr == 0 || params.height * params.width == 0) {
        return {};
    }

    // Check for an exact match in existing surfaces
    auto search = surface_cache.find(params.addr);
    Surface surface;
    if (search != surface_cache.end()) {
        surface = search->second;
    } else {
        surface = std::make_shared<CachedSurface>(params);
        surface_cache[params.addr] = surface;
    }

    LoadSurface(surface);

    return surface;
}
