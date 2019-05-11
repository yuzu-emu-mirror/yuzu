// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <map>

#include "common/alignment.h"
#include "common/bit_util.h"
#include "common/cityhash.h"
#include "core/core.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/surface.h"
#include "video_core/texture_cache/surface_params.h"

namespace VideoCommon {

using VideoCore::Surface::ComponentTypeFromDepthFormat;
using VideoCore::Surface::ComponentTypeFromRenderTarget;
using VideoCore::Surface::ComponentTypeFromTexture;
using VideoCore::Surface::PixelFormatFromDepthFormat;
using VideoCore::Surface::PixelFormatFromRenderTargetFormat;
using VideoCore::Surface::PixelFormatFromTextureFormat;
using VideoCore::Surface::SurfaceTarget;
using VideoCore::Surface::SurfaceTargetFromTextureType;

SurfaceTarget TextureType2SurfaceTarget(Tegra::Shader::TextureType type, bool is_array) {
    switch (type) {
    case Tegra::Shader::TextureType::Texture1D: {
        if (is_array)
            return SurfaceTarget::Texture1DArray;
        else
            return SurfaceTarget::Texture1D;
    }
    case Tegra::Shader::TextureType::Texture2D: {
        if (is_array)
            return SurfaceTarget::Texture2DArray;
        else
            return SurfaceTarget::Texture2D;
    }
    case Tegra::Shader::TextureType::Texture3D: {
        ASSERT(!is_array);
        return SurfaceTarget::Texture3D;
    }
    case Tegra::Shader::TextureType::TextureCube: {
        if (is_array)
            return SurfaceTarget::TextureCubeArray;
        else
            return SurfaceTarget::TextureCubemap;
    }
    default: {
        UNREACHABLE();
        return SurfaceTarget::Texture2D;
    }
    }
}

namespace {
constexpr u32 GetMipmapSize(bool uncompressed, u32 mip_size, u32 tile) {
    return uncompressed ? mip_size : std::max(1U, (mip_size + tile - 1) / tile);
}
} // Anonymous namespace

SurfaceParams SurfaceParams::CreateForTexture(Core::System& system,
                                              const Tegra::Texture::FullTextureInfo& config,
                                              const VideoCommon::Shader::Sampler& entry) {
    SurfaceParams params;
    params.is_tiled = config.tic.IsTiled();
    params.srgb_conversion = config.tic.IsSrgbConversionEnabled();
    params.block_width = params.is_tiled ? config.tic.BlockWidth() : 0,
    params.block_height = params.is_tiled ? config.tic.BlockHeight() : 0,
    params.block_depth = params.is_tiled ? config.tic.BlockDepth() : 0,
    params.tile_width_spacing = params.is_tiled ? (1 << config.tic.tile_width_spacing.Value()) : 1;
    params.pixel_format = PixelFormatFromTextureFormat(config.tic.format, config.tic.r_type.Value(),
                                                       params.srgb_conversion);
    params.component_type = ComponentTypeFromTexture(config.tic.r_type.Value());
    params.type = GetFormatType(params.pixel_format);
    // TODO: on 1DBuffer we should use the tic info.
    params.target = TextureType2SurfaceTarget(entry.GetType(), entry.IsArray());
    params.width =
        Common::AlignBits(config.tic.Width(), GetCompressionFactorShift(params.pixel_format));
    params.height =
        Common::AlignBits(config.tic.Height(), GetCompressionFactorShift(params.pixel_format));
    params.depth = config.tic.Depth();
    if (params.target == SurfaceTarget::TextureCubemap ||
        params.target == SurfaceTarget::TextureCubeArray) {
        params.depth *= 6;
    }
    params.pitch = params.is_tiled ? 0 : config.tic.Pitch();
    params.unaligned_height = config.tic.Height();
    params.num_levels = config.tic.max_mip_level + 1;
    params.is_layered = params.IsLayered();
    return params;
}

SurfaceParams SurfaceParams::CreateForDepthBuffer(
    Core::System& system, u32 zeta_width, u32 zeta_height, Tegra::DepthFormat format,
    u32 block_width, u32 block_height, u32 block_depth,
    Tegra::Engines::Maxwell3D::Regs::InvMemoryLayout type) {
    SurfaceParams params;
    params.is_tiled = type == Tegra::Engines::Maxwell3D::Regs::InvMemoryLayout::BlockLinear;
    params.srgb_conversion = false;
    params.block_width = std::min(block_width, 5U);
    params.block_height = std::min(block_height, 5U);
    params.block_depth = std::min(block_depth, 5U);
    params.tile_width_spacing = 1;
    params.pixel_format = PixelFormatFromDepthFormat(format);
    params.component_type = ComponentTypeFromDepthFormat(format);
    params.type = GetFormatType(params.pixel_format);
    params.width = zeta_width;
    params.height = zeta_height;
    params.unaligned_height = zeta_height;
    params.target = SurfaceTarget::Texture2D;
    params.depth = 1;
    params.num_levels = 1;
    params.is_layered = false;
    return params;
}

SurfaceParams SurfaceParams::CreateForFramebuffer(Core::System& system, std::size_t index) {
    const auto& config{system.GPU().Maxwell3D().regs.rt[index]};
    SurfaceParams params;
    params.is_tiled =
        config.memory_layout.type == Tegra::Engines::Maxwell3D::Regs::InvMemoryLayout::BlockLinear;
    params.srgb_conversion = config.format == Tegra::RenderTargetFormat::BGRA8_SRGB ||
                             config.format == Tegra::RenderTargetFormat::RGBA8_SRGB;
    params.block_width = config.memory_layout.block_width;
    params.block_height = config.memory_layout.block_height;
    params.block_depth = config.memory_layout.block_depth;
    params.tile_width_spacing = 1;
    params.pixel_format = PixelFormatFromRenderTargetFormat(config.format);
    params.component_type = ComponentTypeFromRenderTarget(config.format);
    params.type = GetFormatType(params.pixel_format);
    if (params.is_tiled) {
        params.width = config.width;
    } else {
        const u32 bpp = GetFormatBpp(params.pixel_format) / CHAR_BIT;
        params.pitch = config.width;
        params.width = params.pitch / bpp;
    }
    params.height = config.height;
    params.depth = 1;
    params.unaligned_height = config.height;
    params.target = SurfaceTarget::Texture2D;
    params.num_levels = 1;
    params.is_layered = false;
    return params;
}

SurfaceParams SurfaceParams::CreateForFermiCopySurface(
    const Tegra::Engines::Fermi2D::Regs::Surface& config) {
    SurfaceParams params{};
    params.is_tiled = !config.linear;
    params.srgb_conversion = config.format == Tegra::RenderTargetFormat::BGRA8_SRGB ||
                             config.format == Tegra::RenderTargetFormat::RGBA8_SRGB;
    params.block_width = params.is_tiled ? std::min(config.BlockWidth(), 5U) : 0,
    params.block_height = params.is_tiled ? std::min(config.BlockHeight(), 5U) : 0,
    params.block_depth = params.is_tiled ? std::min(config.BlockDepth(), 5U) : 0,
    params.tile_width_spacing = 1;
    params.pixel_format = PixelFormatFromRenderTargetFormat(config.format);
    params.component_type = ComponentTypeFromRenderTarget(config.format);
    params.type = GetFormatType(params.pixel_format);
    params.width = config.width;
    params.height = config.height;
    params.pitch = config.pitch;
    params.unaligned_height = config.height;
    // TODO(Rodrigo): Try to guess the surface target from depth and layer parameters
    params.target = SurfaceTarget::Texture2D;
    params.depth = 1;
    params.num_levels = 1;
    params.is_layered = params.IsLayered();
    return params;
}

bool SurfaceParams::IsLayered() const {
    switch (target) {
    case SurfaceTarget::Texture1DArray:
    case SurfaceTarget::Texture2DArray:
    case SurfaceTarget::TextureCubemap:
    case SurfaceTarget::TextureCubeArray:
        return true;
    default:
        return false;
    }
}

u32 SurfaceParams::GetMipBlockHeight(u32 level) const {
    // Auto block resizing algorithm from:
    // https://cgit.freedesktop.org/mesa/mesa/tree/src/gallium/drivers/nouveau/nv50/nv50_miptree.c
    if (level == 0) {
        return this->block_height;
    }

    const u32 height{GetMipHeight(level)};
    const u32 default_block_height{GetDefaultBlockHeight()};
    const u32 blocks_in_y{(height + default_block_height - 1) / default_block_height};
    const u32 block_height = Common::Log2Ceil32(blocks_in_y);
    return std::clamp(block_height, 3U, 8U) - 3U;
}

u32 SurfaceParams::GetMipBlockDepth(u32 level) const {
    if (level == 0) {
        return this->block_depth;
    }
    if (is_layered) {
        return 0;
    }

    const u32 depth{GetMipDepth(level)};
    const u32 block_depth = Common::Log2Ceil32(depth);
    if (block_depth > 4) {
        return 5 - (GetMipBlockHeight(level) >= 2);
    }
    return block_depth;
}

std::size_t SurfaceParams::GetGuestMipmapLevelOffset(u32 level) const {
    std::size_t offset = 0;
    for (u32 i = 0; i < level; i++) {
        offset += GetInnerMipmapMemorySize(i, false, false);
    }
    return offset;
}

std::size_t SurfaceParams::GetHostMipmapLevelOffset(u32 level) const {
    std::size_t offset = 0;
    for (u32 i = 0; i < level; i++) {
        offset += GetInnerMipmapMemorySize(i, true, false) * GetNumLayers();
    }
    return offset;
}

std::size_t SurfaceParams::GetGuestMipmapSize(u32 level) const {
    return GetInnerMipmapMemorySize(level, false, false);
}

std::size_t SurfaceParams::GetHostMipmapSize(u32 level) const {
    return GetInnerMipmapMemorySize(level, true, false) * GetNumLayers();
}

std::size_t SurfaceParams::GetGuestLayerSize() const {
    return GetLayerSize(false, false);
}

std::size_t SurfaceParams::GetLayerSize(bool as_host_size, bool uncompressed) const {
    std::size_t size = 0;
    for (u32 level = 0; level < num_levels; ++level) {
        size += GetInnerMipmapMemorySize(level, as_host_size, uncompressed);
    }
    if (is_tiled && is_layered) {
        return Common::AlignBits(size,
                                 Tegra::Texture::GetGOBSizeShift() + block_height + block_depth);
    }
    return size;
}

std::size_t SurfaceParams::GetHostLayerSize(u32 level) const {
    ASSERT(target != SurfaceTarget::Texture3D);
    return GetInnerMipmapMemorySize(level, true, false);
}

bool SurfaceParams::IsPixelFormatZeta() const {
    return pixel_format >= VideoCore::Surface::PixelFormat::MaxColorFormat &&
           pixel_format < VideoCore::Surface::PixelFormat::MaxDepthStencilFormat;
}

std::size_t SurfaceParams::GetInnerMipmapMemorySize(u32 level, bool as_host_size,
                                                    bool uncompressed) const {
    const bool tiled{as_host_size ? false : is_tiled};
    const u32 width{GetMipmapSize(uncompressed, GetMipWidth(level), GetDefaultBlockWidth())};
    const u32 height{GetMipmapSize(uncompressed, GetMipHeight(level), GetDefaultBlockHeight())};
    const u32 depth{is_layered ? 1U : GetMipDepth(level)};
    return Tegra::Texture::CalculateSize(tiled, GetBytesPerPixel(), width, height, depth,
                                         GetMipBlockHeight(level), GetMipBlockDepth(level));
}

std::size_t SurfaceParams::GetInnerMemorySize(bool as_host_size, bool layer_only,
                                              bool uncompressed) const {
    return GetLayerSize(as_host_size, uncompressed) * (layer_only ? 1U : depth);
}

std::size_t SurfaceParams::Hash() const {
    return static_cast<std::size_t>(
        Common::CityHash64(reinterpret_cast<const char*>(this), sizeof(*this)));
}

bool SurfaceParams::operator==(const SurfaceParams& rhs) const {
    return std::tie(is_tiled, block_width, block_height, block_depth, tile_width_spacing, width,
                    height, depth, pitch, unaligned_height, num_levels, pixel_format,
                    component_type, type, target) ==
           std::tie(rhs.is_tiled, rhs.block_width, rhs.block_height, rhs.block_depth,
                    rhs.tile_width_spacing, rhs.width, rhs.height, rhs.depth, rhs.pitch,
                    rhs.unaligned_height, rhs.num_levels, rhs.pixel_format, rhs.component_type,
                    rhs.type, rhs.target);
}

std::string SurfaceParams::TargetName() const {
    switch (target) {
    case SurfaceTarget::Texture1D:
        return "1D";
    case SurfaceTarget::Texture2D:
        return "2D";
    case SurfaceTarget::Texture3D:
        return "3D";
    case SurfaceTarget::Texture1DArray:
        return "1DArray";
    case SurfaceTarget::Texture2DArray:
        return "2DArray";
    case SurfaceTarget::TextureCubemap:
        return "Cube";
    case SurfaceTarget::TextureCubeArray:
        return "CubeArray";
    default:
        LOG_CRITICAL(HW_GPU, "Unimplemented surface_target={}", static_cast<u32>(target));
        UNREACHABLE();
        return fmt::format("TUK({})", static_cast<u32>(target));
    }
}

} // namespace VideoCommon
