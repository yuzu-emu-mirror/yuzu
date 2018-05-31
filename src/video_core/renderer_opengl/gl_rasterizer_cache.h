// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>
#include <set>
#include <tuple>
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif
#include <boost/icl/interval_map.hpp>
#include <boost/icl/interval_set.hpp>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include <boost/optional.hpp>
#include <glad/glad.h>
#include "common/assert.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/math_util.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/textures/texture.h"

struct CachedSurface;
using Surface = std::shared_ptr<CachedSurface>;
using SurfaceSet = std::set<Surface>;

using SurfaceRegions = boost::icl::interval_set<Tegra::GPUVAddr>;
using SurfaceMap = boost::icl::interval_map<Tegra::GPUVAddr, Surface>;
using SurfaceCache = boost::icl::interval_map<Tegra::GPUVAddr, SurfaceSet>;

using SurfaceInterval = SurfaceCache::interval_type;
static_assert(std::is_same<SurfaceRegions::interval_type, SurfaceCache::interval_type>() &&
                  std::is_same<SurfaceMap::interval_type, SurfaceCache::interval_type>(),
              "incorrect interval types");

using SurfaceRect_Tuple = std::tuple<Surface, MathUtil::Rectangle<u32>>;
using SurfaceSurfaceRect_Tuple = std::tuple<Surface, Surface, MathUtil::Rectangle<u32>>;

using PageMap = boost::icl::interval_map<u64, int>;

enum class ScaleMatch {
    Exact,   // only accept same res scale
    Upscale, // only allow higher scale than params
    Ignore   // accept every scaled res
};

struct SurfaceParams {
    enum class PixelFormat {
        ABGR8 = 0,
        B5G6R5 = 1,
        A2B10G10R10 = 2,
        A1B5G5R5 = 3,
        R8 = 4,
        RGBA16F = 5,
        DXT1 = 6,
        DXT23 = 7,
        DXT45 = 8,

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
     * compressed image. This is used for maintaining proper surface sizes for compressed texture
     * formats.
     */
    static constexpr u32 GetCompresssionFactor(PixelFormat format) {
        if (format == PixelFormat::Invalid)
            return 0;

        constexpr std::array<u32, MaxPixelFormat> compression_factor_table = {{
            1, // ABGR8
            1, // B5G6R5
            1, // A2B10G10R10
            1, // A1B5G5R5
            1, // R8
            2, // RGBA16F
            4, // DXT1
            4, // DXT23
            4, // DXT45
        }};

        ASSERT(static_cast<size_t>(format) < compression_factor_table.size());
        return compression_factor_table[static_cast<size_t>(format)];
    }
    u32 GetCompresssionFactor() const {
        return GetCompresssionFactor(pixel_format);
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
            64,  // DXT1
            128, // DXT23
            128, // DXT45
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
        case Tegra::Texture::TextureFormat::DXT1:
            return PixelFormat::DXT1;
        case Tegra::Texture::TextureFormat::DXT23:
            return PixelFormat::DXT23;
        case Tegra::Texture::TextureFormat::DXT45:
            return PixelFormat::DXT45;
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
        case PixelFormat::DXT1:
            return Tegra::Texture::TextureFormat::DXT1;
        case PixelFormat::DXT23:
            return Tegra::Texture::TextureFormat::DXT23;
        case PixelFormat::DXT45:
            return Tegra::Texture::TextureFormat::DXT45;
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
            return ComponentType::Float;
        default:
            NGLOG_CRITICAL(HW_GPU, "Unimplemented format={}", static_cast<u32>(format));
            UNREACHABLE();
        }
    }

    static ComponentType ComponentTypeFromGPUPixelFormat(
        Tegra::FramebufferConfig::PixelFormat format) {
        switch (format) {
        case Tegra::FramebufferConfig::PixelFormat::ABGR8:
            return ComponentType::UNorm;
        default:
            NGLOG_CRITICAL(HW_GPU, "Unimplemented format={}", static_cast<u32>(format));
            UNREACHABLE();
        }
    }

    static bool CheckFormatsBlittable(PixelFormat pixel_format_a, PixelFormat pixel_format_b) {
        SurfaceType a_type = GetFormatType(pixel_format_a);
        SurfaceType b_type = GetFormatType(pixel_format_b);

        if (a_type == SurfaceType::ColorTexture && b_type == SurfaceType::ColorTexture) {
            return true;
        }

        if (a_type == SurfaceType::Depth && b_type == SurfaceType::Depth) {
            return true;
        }

        if (a_type == SurfaceType::DepthStencil && b_type == SurfaceType::DepthStencil) {
            return true;
        }

        return false;
    }

    static SurfaceType GetFormatType(PixelFormat pixel_format) {
        if (static_cast<size_t>(pixel_format) < MaxPixelFormat) {
            return SurfaceType::ColorTexture;
        }

        // TODO(Subv): Implement the other formats
        ASSERT(false);

        return SurfaceType::Invalid;
    }

    /// Update the params "size", "end" and "type" from the already set "addr", "width", "height"
    /// and "pixel_format"
    void UpdateParams() {
        if (stride == 0) {
            stride = width;
        }
        type = GetFormatType(pixel_format);
        size = !is_tiled ? BytesInPixels(stride * (height - 1) + width)
                         : BytesInPixels(stride * 8 * (height / 8 - 1) + width * 8);
        end = addr + size;
    }

    SurfaceInterval GetInterval() const {
        return SurfaceInterval::right_open(addr, end);
    }

    // Returns the outer rectangle containing "interval"
    SurfaceParams FromInterval(SurfaceInterval interval) const;

    SurfaceInterval GetSubRectInterval(MathUtil::Rectangle<u32> unscaled_rect) const;

    // Returns the region of the biggest valid rectange within interval
    SurfaceInterval GetCopyableInterval(const Surface& src_surface) const;

    /**
     * Gets the actual width (in pixels) of the surface. This is provided because `width` is used
     * for tracking the surface region in memory, which may be compressed for certain formats. In
     * this scenario, `width` is actually the compressed width.
     */
    u32 GetActualWidth() const {
        return width * GetCompresssionFactor();
    }

    /**
     * Gets the actual height (in pixels) of the surface. This is provided because `height` is used
     * for tracking the surface region in memory, which may be compressed for certain formats. In
     * this scenario, `height` is actually the compressed height.
     */
    u32 GetActualHeight() const {
        return height * GetCompresssionFactor();
    }

    u32 GetScaledWidth() const {
        return width * res_scale;
    }

    u32 GetScaledHeight() const {
        return height * res_scale;
    }

    MathUtil::Rectangle<u32> GetRect() const {
        return {0, height, width, 0};
    }

    MathUtil::Rectangle<u32> GetScaledRect() const {
        return {0, GetScaledHeight(), GetScaledWidth(), 0};
    }

    u64 PixelsInBytes(u64 size) const {
        return size * CHAR_BIT / GetFormatBpp(pixel_format);
    }

    u64 BytesInPixels(u64 pixels) const {
        return pixels * GetFormatBpp(pixel_format) / CHAR_BIT;
    }

    VAddr GetCpuAddr() const;

    bool ExactMatch(const SurfaceParams& other_surface) const;
    bool CanSubRect(const SurfaceParams& sub_surface) const;
    bool CanExpand(const SurfaceParams& expanded_surface) const;
    bool CanTexCopy(const SurfaceParams& texcopy_params) const;

    MathUtil::Rectangle<u32> GetSubRect(const SurfaceParams& sub_surface) const;
    MathUtil::Rectangle<u32> GetScaledSubRect(const SurfaceParams& sub_surface) const;

    Tegra::GPUVAddr addr = 0;
    Tegra::GPUVAddr end = 0;
    boost::optional<VAddr> cpu_addr;
    u64 size = 0;

    u32 width = 0;
    u32 height = 0;
    u32 stride = 0;
    u32 block_height = 0;
    u16 res_scale = 1;

    bool is_tiled = false;
    PixelFormat pixel_format = PixelFormat::Invalid;
    SurfaceType type = SurfaceType::Invalid;
    ComponentType component_type = ComponentType::Invalid;
};

struct CachedSurface : SurfaceParams {
    bool CanFill(const SurfaceParams& dest_surface, SurfaceInterval fill_interval) const;
    bool CanCopy(const SurfaceParams& dest_surface, SurfaceInterval copy_interval) const;

    bool IsRegionValid(SurfaceInterval interval) const {
        return (invalid_regions.find(interval) == invalid_regions.end());
    }

    bool IsSurfaceFullyInvalid() const {
        return (invalid_regions & GetInterval()) == SurfaceRegions(GetInterval());
    }

    bool registered = false;
    SurfaceRegions invalid_regions;

    u64 fill_size = 0; /// Number of bytes to read from fill_data
    std::array<u8, 4> fill_data;

    OGLTexture texture;

    static constexpr unsigned int GetGLBytesPerPixel(PixelFormat format) {
        if (format == PixelFormat::Invalid)
            return 0;

        return SurfaceParams::GetFormatBpp(format) / CHAR_BIT;
    }

    std::unique_ptr<u8[]> gl_buffer;
    size_t gl_buffer_size = 0;

    // Read/Write data in Switch memory to/from gl_buffer
    void LoadGLBuffer(Tegra::GPUVAddr load_start, Tegra::GPUVAddr load_end);
    void FlushGLBuffer(Tegra::GPUVAddr flush_start, Tegra::GPUVAddr flush_end);

    // Upload/Download data in gl_buffer in/to this surface's texture
    void UploadGLTexture(const MathUtil::Rectangle<u32>& rect, GLuint read_fb_handle,
                         GLuint draw_fb_handle);
    void DownloadGLTexture(const MathUtil::Rectangle<u32>& rect, GLuint read_fb_handle,
                           GLuint draw_fb_handle);
};

class RasterizerCacheOpenGL : NonCopyable {
public:
    RasterizerCacheOpenGL();
    ~RasterizerCacheOpenGL();

    /// Blit one surface's texture to another
    bool BlitSurfaces(const Surface& src_surface, const MathUtil::Rectangle<u32>& src_rect,
                      const Surface& dst_surface, const MathUtil::Rectangle<u32>& dst_rect);

    void ConvertD24S8toABGR(GLuint src_tex, const MathUtil::Rectangle<u32>& src_rect,
                            GLuint dst_tex, const MathUtil::Rectangle<u32>& dst_rect);

    /// Copy one surface's region to another
    void CopySurface(const Surface& src_surface, const Surface& dst_surface,
                     SurfaceInterval copy_interval);

    /// Load a texture from Switch memory to OpenGL and cache it (if not already cached)
    Surface GetSurface(const SurfaceParams& params, ScaleMatch match_res_scale,
                       bool load_if_create);

    /// Tries to find a framebuffer GPU address based on the provided CPU address
    boost::optional<Tegra::GPUVAddr> TryFindFramebufferGpuAddress(VAddr cpu_addr) const;

    /// Attempt to find a subrect (resolution scaled) of a surface, otherwise loads a texture from
    /// Switch memory to OpenGL and caches it (if not already cached)
    SurfaceRect_Tuple GetSurfaceSubRect(const SurfaceParams& params, ScaleMatch match_res_scale,
                                        bool load_if_create);

    /// Get a surface based on the texture configuration
    Surface GetTextureSurface(const Tegra::Texture::FullTextureInfo& config);

    /// Get the color and depth surfaces based on the framebuffer configuration
    SurfaceSurfaceRect_Tuple GetFramebufferSurfaces(bool using_color_fb, bool using_depth_fb,
                                                    const MathUtil::Rectangle<s32>& viewport);

    /// Get a surface that matches the fill config
    Surface GetFillSurface(const void* config);

    /// Get a surface that matches a "texture copy" display transfer config
    SurfaceRect_Tuple GetTexCopySurface(const SurfaceParams& params);

    /// Write any cached resources overlapping the region back to memory (if dirty)
    void FlushRegion(Tegra::GPUVAddr addr, u64 size, Surface flush_surface = nullptr);

    /// Mark region as being invalidated by region_owner (nullptr if Switch memory)
    void InvalidateRegion(Tegra::GPUVAddr addr, u64 size, const Surface& region_owner);

    /// Flush all cached resources tracked by this cache manager
    void FlushAll();

private:
    void DuplicateSurface(const Surface& src_surface, const Surface& dest_surface);

    /// Update surface's texture for given region when necessary
    void ValidateSurface(const Surface& surface, Tegra::GPUVAddr addr, u64 size);

    /// Create a new surface
    Surface CreateSurface(const SurfaceParams& params);

    /// Register surface into the cache
    void RegisterSurface(const Surface& surface);

    /// Remove surface from the cache
    void UnregisterSurface(const Surface& surface);

    /// Increase/decrease the number of surface in pages touching the specified region
    void UpdatePagesCachedCount(Tegra::GPUVAddr addr, u64 size, int delta);

    SurfaceCache surface_cache;
    PageMap cached_pages;
    SurfaceMap dirty_regions;
    SurfaceSet remove_surfaces;

    OGLFramebuffer read_framebuffer;
    OGLFramebuffer draw_framebuffer;

    OGLVertexArray attributeless_vao;
    OGLBuffer d24s8_abgr_buffer;
    GLsizeiptr d24s8_abgr_buffer_size;
    OGLProgram d24s8_abgr_shader;
    GLint d24s8_abgr_tbo_size_u_id;
    GLint d24s8_abgr_viewport_u_id;
};
