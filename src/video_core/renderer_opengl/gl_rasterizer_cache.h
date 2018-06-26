// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <map>
#include <memory>

#include "common/common_types.h"
#include "common/hash.h"
#include "common/math_util.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/textures/texture.h"

class CachedSurface;
using Surface = std::shared_ptr<CachedSurface>;
using SurfaceSurfaceRect_Tuple = std::tuple<Surface, Surface, MathUtil::Rectangle<u32>>;

struct SurfaceParams {
    enum class PixelFormat {
        ABGR8 = 0,
        B5G6R5 = 1,
        A2B10G10R10 = 2,
        A1B5G5R5 = 3,
        R8 = 4,
        RGBA16F = 5,
        R11FG11FB10F = 6,
        DXT1 = 7,
        DXT23 = 8,
        DXT45 = 9,
        DXN1 = 10, // This is also known as BC4
        ASTC_2D_4X4 = 11,

        Max,
        Invalid = 255,
    };

    static constexpr size_t MaxPixelFormat = static_cast<size_t>(PixelFormat::Max);

    enum class ComponentType {
        Invalid = 0,
        SNorm = 1,
        UNorm = 2,
        SInt = 3,
        UInt = 4,
        Float = 5,
    };

    enum class SurfaceType {
        ColorTexture = 0,
        Depth = 1,
        DepthStencil = 2,
        Fill = 3,
        Invalid = 4,
    };

    /**
     * Gets the compression factor for the specified PixelFormat. This applies to just the
     * "compressed width" and "compressed height", not the overall compression factor of a
     * compressed image. This is used for maintaining proper surface sizes for compressed
     * texture formats.
     */
    static constexpr u32 GetCompressionFactor(PixelFormat format) {
        if (format == PixelFormat::Invalid)
            return 0;

        constexpr std::array<u32, MaxPixelFormat> compression_factor_table = {{
            1, // ABGR8
            1, // B5G6R5
            1, // A2B10G10R10
            1, // A1B5G5R5
            1, // R8
            1, // RGBA16F
            1, // R11FG11FB10F
            4, // DXT1
            4, // DXT23
            4, // DXT45
            4, // DXN1
            1, // ASTC_2D_4X4
        }};

        ASSERT(static_cast<size_t>(format) < compression_factor_table.size());
        return compression_factor_table[static_cast<size_t>(format)];
    }

    static constexpr u32 GetFormatBpp(PixelFormat format) {
        if (format == PixelFormat::Invalid)
            return 0;

        constexpr std::array<u32, MaxPixelFormat> bpp_table = {{
            32,  // ABGR8
            16,  // B5G6R5
            32,  // A2B10G10R10
            16,  // A1B5G5R5
            8,   // R8
            64,  // RGBA16F
            32,  // R11FG11FB10F
            64,  // DXT1
            128, // DXT23
            128, // DXT45
            64,  // DXN1
            32,  // ASTC_2D_4X4
        }};

        ASSERT(static_cast<size_t>(format) < bpp_table.size());
        return bpp_table[static_cast<size_t>(format)];
    }
    u32 GetFormatBpp() const {
        return GetFormatBpp(pixel_format);
    }

    static PixelFormat PixelFormatFromRenderTargetFormat(Tegra::RenderTargetFormat format) {
        switch (format) {
        case Tegra::RenderTargetFormat::RGBA8_UNORM:
        case Tegra::RenderTargetFormat::RGBA8_SRGB:
            return PixelFormat::ABGR8;
        case Tegra::RenderTargetFormat::RGB10_A2_UNORM:
            return PixelFormat::A2B10G10R10;
        case Tegra::RenderTargetFormat::RGBA16_FLOAT:
            return PixelFormat::RGBA16F;
        case Tegra::RenderTargetFormat::R11G11B10_FLOAT:
            return PixelFormat::R11FG11FB10F;
        default:
            NGLOG_CRITICAL(HW_GPU, "Unimplemented format={}", static_cast<u32>(format));
            UNREACHABLE();
        }
    }

    static PixelFormat PixelFormatFromTextureFormat(Tegra::Texture::TextureFormat format) {
        // TODO(Subv): Properly implement this
        switch (format) {
        case Tegra::Texture::TextureFormat::A8R8G8B8:
            return PixelFormat::ABGR8;
        case Tegra::Texture::TextureFormat::B5G6R5:
            return PixelFormat::B5G6R5;
        case Tegra::Texture::TextureFormat::A2B10G10R10:
            return PixelFormat::A2B10G10R10;
        case Tegra::Texture::TextureFormat::A1B5G5R5:
            return PixelFormat::A1B5G5R5;
        case Tegra::Texture::TextureFormat::R8:
            return PixelFormat::R8;
        case Tegra::Texture::TextureFormat::R16_G16_B16_A16:
            return PixelFormat::RGBA16F;
        case Tegra::Texture::TextureFormat::BF10GF11RF11:
            return PixelFormat::R11FG11FB10F;
        case Tegra::Texture::TextureFormat::DXT1:
            return PixelFormat::DXT1;
        case Tegra::Texture::TextureFormat::DXT23:
            return PixelFormat::DXT23;
        case Tegra::Texture::TextureFormat::DXT45:
            return PixelFormat::DXT45;
        case Tegra::Texture::TextureFormat::DXN1:
            return PixelFormat::DXN1;
        case Tegra::Texture::TextureFormat::ASTC_2D_4X4:
            return PixelFormat::ASTC_2D_4X4;
        default:
            NGLOG_CRITICAL(HW_GPU, "Unimplemented format={}", static_cast<u32>(format));
            UNREACHABLE();
        }
    }

    static Tegra::Texture::TextureFormat TextureFormatFromPixelFormat(PixelFormat format) {
        // TODO(Subv): Properly implement this
        switch (format) {
        case PixelFormat::ABGR8:
            return Tegra::Texture::TextureFormat::A8R8G8B8;
        case PixelFormat::B5G6R5:
            return Tegra::Texture::TextureFormat::B5G6R5;
        case PixelFormat::A2B10G10R10:
            return Tegra::Texture::TextureFormat::A2B10G10R10;
        case PixelFormat::A1B5G5R5:
            return Tegra::Texture::TextureFormat::A1B5G5R5;
        case PixelFormat::R8:
            return Tegra::Texture::TextureFormat::R8;
        case PixelFormat::RGBA16F:
            return Tegra::Texture::TextureFormat::R16_G16_B16_A16;
        case PixelFormat::R11FG11FB10F:
            return Tegra::Texture::TextureFormat::BF10GF11RF11;
        case PixelFormat::DXT1:
            return Tegra::Texture::TextureFormat::DXT1;
        case PixelFormat::DXT23:
            return Tegra::Texture::TextureFormat::DXT23;
        case PixelFormat::DXT45:
            return Tegra::Texture::TextureFormat::DXT45;
        case PixelFormat::DXN1:
            return Tegra::Texture::TextureFormat::DXN1;
        case PixelFormat::ASTC_2D_4X4:
            return Tegra::Texture::TextureFormat::ASTC_2D_4X4;
        default:
            UNREACHABLE();
        }
    }

    static ComponentType ComponentTypeFromTexture(Tegra::Texture::ComponentType type) {
        // TODO(Subv): Implement more component types
        switch (type) {
        case Tegra::Texture::ComponentType::UNORM:
            return ComponentType::UNorm;
        default:
            NGLOG_CRITICAL(HW_GPU, "Unimplemented component type={}", static_cast<u32>(type));
            UNREACHABLE();
        }
    }

    static ComponentType ComponentTypeFromRenderTarget(Tegra::RenderTargetFormat format) {
        // TODO(Subv): Implement more render targets
        switch (format) {
        case Tegra::RenderTargetFormat::RGBA8_UNORM:
        case Tegra::RenderTargetFormat::RGBA8_SRGB:
        case Tegra::RenderTargetFormat::RGB10_A2_UNORM:
            return ComponentType::UNorm;
        case Tegra::RenderTargetFormat::RGBA16_FLOAT:
        case Tegra::RenderTargetFormat::R11G11B10_FLOAT:
            return ComponentType::Float;
        default:
            NGLOG_CRITICAL(HW_GPU, "Unimplemented format={}", static_cast<u32>(format));
            UNREACHABLE();
        }
    }

    static PixelFormat PixelFormatFromGPUPixelFormat(Tegra::FramebufferConfig::PixelFormat format) {
        switch (format) {
        case Tegra::FramebufferConfig::PixelFormat::ABGR8:
            return PixelFormat::ABGR8;
        default:
            NGLOG_CRITICAL(HW_GPU, "Unimplemented format={}", static_cast<u32>(format));
            UNREACHABLE();
        }
    }

    static SurfaceType GetFormatType(PixelFormat pixel_format) {
        if (static_cast<size_t>(pixel_format) < MaxPixelFormat) {
            return SurfaceType::ColorTexture;
        }

        // TODO(Subv): Implement the other formats
        ASSERT(false);

        return SurfaceType::Invalid;
    }

    MathUtil::Rectangle<u32> GetRect() const {
        return {0, height, width, 0};
    }

    size_t SizeInBytes() const {
        const u32 compression_factor{GetCompressionFactor(pixel_format)};
        ASSERT(width % compression_factor == 0);
        ASSERT(height % compression_factor == 0);
        return (width / compression_factor) * (height / compression_factor) *
               GetFormatBpp(pixel_format) / CHAR_BIT;
    }

    VAddr GetCpuAddr() const;

    static SurfaceParams CreateForTexture(const Tegra::Texture::FullTextureInfo& config);

    static SurfaceParams CreateForFramebuffer(
        const Tegra::Engines::Maxwell3D::Regs::RenderTargetConfig& config);

    Tegra::GPUVAddr addr;
    bool is_tiled;
    u32 block_height;
    PixelFormat pixel_format;
    ComponentType component_type;
    SurfaceType type;
    u32 width;
    u32 height;
    size_t size_in_bytes;
};

struct SurfaceKey : Common::HashableStruct<SurfaceParams> {
    static SurfaceKey Create(const SurfaceParams& params) {
        SurfaceKey res;
        res.state = params;
        return res;
    }
};

namespace std {
template <>
struct hash<SurfaceKey> {
    size_t operator()(const SurfaceKey& k) const {
        return k.Hash();
    }
};
} // namespace std

class CachedSurface final {
public:
    CachedSurface(const SurfaceParams& params);

    const OGLTexture& Texture() const {
        return texture;
    }

    static constexpr unsigned int GetGLBytesPerPixel(SurfaceParams::PixelFormat format) {
        if (format == SurfaceParams::PixelFormat::Invalid)
            return 0;

        return SurfaceParams::GetFormatBpp(format) / CHAR_BIT;
    }

    const SurfaceParams& GetSurfaceParams() const {
        return params;
    }

    // Read/Write data in Switch memory to/from gl_buffer
    void LoadGLBuffer();
    void FlushGLBuffer();

    // Upload/Download data in gl_buffer in/to this surface's texture
    void UploadGLTexture(GLuint read_fb_handle, GLuint draw_fb_handle);
    void DownloadGLTexture(GLuint read_fb_handle, GLuint draw_fb_handle);

private:
    OGLTexture texture;
    std::unique_ptr<u8[]> gl_buffer;
    size_t gl_buffer_size;
    SurfaceParams params;
};

class RasterizerCacheOpenGL final : NonCopyable {
public:
    RasterizerCacheOpenGL();

    Surface GetTextureSurface(const Tegra::Texture::FullTextureInfo& config);
    SurfaceSurfaceRect_Tuple GetFramebufferSurfaces(bool using_color_fb, bool using_depth_fb,
                                                    const MathUtil::Rectangle<s32>& viewport);
    void LoadSurface(const Surface& surface);
    void FlushSurface(const Surface& surface);
    Surface TryFindFramebufferSurface(VAddr cpu_addr) const;

private:
    Surface GetSurface(const SurfaceParams& params);

    std::unordered_map<SurfaceKey, Surface> surface_cache;
    OGLFramebuffer read_framebuffer;
    OGLFramebuffer draw_framebuffer;
};
