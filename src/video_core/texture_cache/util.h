// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <optional>
#include <span>

#include "common/common_types.h"

#include "video_core/engines/maxwell_3d.h"
#include "video_core/surface.h"
#include "video_core/texture_cache/image_base.h"
#include "video_core/texture_cache/image_view_base.h"
#include "video_core/texture_cache/types.h"
#include "video_core/textures/texture.h"

namespace VideoCommon {

using Tegra::Texture::TICEntry;

using LevelArray = std::array<u32, MAX_MIP_LEVELS>;

struct OverlapResult {
    GPUVAddr gpu_addr;
    VAddr cpu_addr;
    SubresourceExtent resources;
};

// This ought to be enough for anybody
constexpr size_t MAX_GUEST_SIZE = 0x4000000;

struct LevelInfo {
    Extent3D size;
    Extent3D block;
    Extent2D tile_size;
    u32 bpp_log2;
    u32 tile_width_spacing;
};

[[nodiscard]] u32 AdjustTileSize(u32 shift, u32 unit_factor, u32 dimension);
[[nodiscard]] u32 AdjustMipSize(u32 size, u32 level);
[[nodiscard]] Extent3D AdjustMipSize(Extent3D size, s32 level);
[[nodiscard]] Extent3D AdjustSamplesSize(Extent3D size, s32 num_samples);
template <u32 GOB_EXTENT>
[[nodiscard]] u32 AdjustMipBlockSize(u32 num_tiles, u32 block_size, u32 level);
[[nodiscard]] Extent3D AdjustMipBlockSize(Extent3D num_tiles, Extent3D block_size, u32 level);
[[nodiscard]] Extent3D AdjustTileSize(Extent3D size, Extent2D tile_size);
[[nodiscard]] u32 BytesPerBlockLog2(u32 bytes_per_block);
[[nodiscard]] u32 BytesPerBlockLog2(PixelFormat format);
[[nodiscard]] u32 NumBlocks(Extent3D size, Extent2D tile_size);
[[nodiscard]] u32 AdjustSize(u32 size, u32 level, u32 block_size);
[[nodiscard]] Extent2D DefaultBlockSize(PixelFormat format);
[[nodiscard]] Extent3D NumLevelBlocks(const LevelInfo& info, u32 level);
[[nodiscard]] Extent3D TileShift(const LevelInfo& info, u32 level);
[[nodiscard]] Extent2D GobSize(u32 bpp_log2, u32 block_height, u32 tile_width_spacing);
[[nodiscard]] bool IsSmallerThanGobSize(Extent3D num_tiles, Extent2D gob, u32 block_depth);
[[nodiscard]] u32 StrideAlignment(Extent3D num_tiles, Extent3D block, Extent2D gob, u32 bpp_log2);
[[nodiscard]] u32 StrideAlignment(Extent3D num_tiles, Extent3D block, u32 bpp_log2,
                                  u32 tile_width_spacing);
[[nodiscard]] Extent2D NumGobs(const LevelInfo& info, u32 level);
[[nodiscard]] Extent3D LevelTiles(const LevelInfo& info, u32 level);
[[nodiscard]] u32 CalculateLevelSize(const LevelInfo& info, u32 level);
[[nodiscard]] LevelArray CalculateLevelSizes(const LevelInfo& info, u32 num_levels);
[[nodiscard]] u32 CalculateLevelBytes(const LevelArray& sizes, u32 num_levels);
[[nodiscard]] LevelInfo MakeLevelInfo(PixelFormat format, Extent3D size, Extent3D block,
                                      u32 tile_width_spacing);
[[nodiscard]] LevelInfo MakeLevelInfo(const ImageInfo& info);
[[nodiscard]] u32 CalculateLevelOffset(PixelFormat format, Extent3D size, Extent3D block,
                                       u32 tile_width_spacing, u32 level);
[[nodiscard]] u32 AlignLayerSize(u32 size_bytes, Extent3D size, Extent3D block, u32 tile_size_y,
                                 u32 tile_width_spacing);
[[nodiscard]] std::optional<SubresourceExtent> ResolveOverlapEqualAddress(const ImageInfo& new_info,
                                                                          const ImageBase& overlap,
                                                                          bool strict_size);
[[nodiscard]] std::optional<SubresourceExtent> ResolveOverlapRightAddress3D(
    const ImageInfo& new_info, GPUVAddr gpu_addr, const ImageBase& overlap, bool strict_size);
[[nodiscard]] std::optional<SubresourceExtent> ResolveOverlapRightAddress2D(
    const ImageInfo& new_info, GPUVAddr gpu_addr, const ImageBase& overlap, bool strict_size);
[[nodiscard]] std::optional<OverlapResult> ResolveOverlapRightAddress(const ImageInfo& new_info,
                                                                      GPUVAddr gpu_addr,
                                                                      VAddr cpu_addr,
                                                                      const ImageBase& overlap,
                                                                      bool strict_size);
[[nodiscard]] std::optional<OverlapResult> ResolveOverlapLeftAddress(const ImageInfo& new_info,
                                                                     GPUVAddr gpu_addr,
                                                                     VAddr cpu_addr,
                                                                     const ImageBase& overlap,
                                                                     bool strict_size);
[[nodiscard]] Extent2D PitchLinearAlignedSize(const ImageInfo& info);
[[nodiscard]] Extent3D BlockLinearAlignedSize(const ImageInfo& info, u32 level);
[[nodiscard]] u32 NumBlocksPerLayer(const ImageInfo& info, Extent2D tile_size) noexcept;
[[nodiscard]] u32 NumSlices(const ImageInfo& info) noexcept;
void SwizzlePitchLinearImage(Tegra::MemoryManager& gpu_memory, GPUVAddr gpu_addr,
                             const ImageInfo& info, const BufferImageCopy& copy,
                             std::span<const u8> memory);
void SwizzleBlockLinearImage(Tegra::MemoryManager& gpu_memory, GPUVAddr gpu_addr,
                             const ImageInfo& info, const BufferImageCopy& copy,
                             std::span<const u8> input);

[[nodiscard]] u32 CalculateGuestSizeInBytes(const ImageInfo& info) noexcept;

[[nodiscard]] u32 CalculateUnswizzledSizeBytes(const ImageInfo& info) noexcept;

[[nodiscard]] u32 CalculateConvertedSizeBytes(const ImageInfo& info) noexcept;

[[nodiscard]] u32 CalculateLayerStride(const ImageInfo& info) noexcept;

[[nodiscard]] u32 CalculateLayerSize(const ImageInfo& info) noexcept;

[[nodiscard]] LevelArray CalculateMipLevelOffsets(const ImageInfo& info) noexcept;

[[nodiscard]] LevelArray CalculateMipLevelSizes(const ImageInfo& info) noexcept;

[[nodiscard]] std::vector<u32> CalculateSliceOffsets(const ImageInfo& info);

[[nodiscard]] std::vector<SubresourceBase> CalculateSliceSubresources(const ImageInfo& info);

[[nodiscard]] u32 CalculateLevelStrideAlignment(const ImageInfo& info, u32 level);

[[nodiscard]] VideoCore::Surface::PixelFormat PixelFormatFromTIC(
    const Tegra::Texture::TICEntry& config) noexcept;

[[nodiscard]] ImageViewType RenderTargetImageViewType(const ImageInfo& info) noexcept;

[[nodiscard]] std::vector<ImageCopy> MakeShrinkImageCopies(const ImageInfo& dst,
                                                           const ImageInfo& src,
                                                           SubresourceBase base);

[[nodiscard]] bool IsValidEntry(const Tegra::MemoryManager& gpu_memory, const TICEntry& config);

[[nodiscard]] std::vector<BufferImageCopy> UnswizzleImage(Tegra::MemoryManager& gpu_memory,
                                                          GPUVAddr gpu_addr, const ImageInfo& info,
                                                          std::array<u8, MAX_GUEST_SIZE>& scratch,
                                                          std::span<u8> output);

[[nodiscard]] BufferCopy UploadBufferCopy(Tegra::MemoryManager& gpu_memory, GPUVAddr gpu_addr,
                                          const ImageBase& image, std::span<u8> output);

void ConvertImage(std::span<const u8> input, const ImageInfo& info, std::span<u8> output,
                  std::span<BufferImageCopy> copies);

[[nodiscard]] std::vector<BufferImageCopy> FullDownloadCopies(const ImageInfo& info);

[[nodiscard]] Extent3D MipSize(Extent3D size, u32 level);

[[nodiscard]] Extent3D MipBlockSize(const ImageInfo& info, u32 level);

[[nodiscard]] std::vector<SwizzleParameters> FullUploadSwizzles(const ImageInfo& info);

void SwizzleImage(Tegra::MemoryManager& gpu_memory, GPUVAddr gpu_addr, const ImageInfo& info,
                  std::span<const BufferImageCopy> copies, std::span<const u8> memory);

[[nodiscard]] bool IsBlockLinearSizeCompatible(const ImageInfo& new_info,
                                               const ImageInfo& overlap_info, u32 new_level,
                                               u32 overlap_level, bool strict_size) noexcept;

[[nodiscard]] bool IsPitchLinearSameSize(const ImageInfo& lhs, const ImageInfo& rhs,
                                         bool strict_size) noexcept;

[[nodiscard]] std::optional<OverlapResult> ResolveOverlap(const ImageInfo& new_info,
                                                          GPUVAddr gpu_addr, VAddr cpu_addr,
                                                          const ImageBase& overlap,
                                                          bool strict_size, bool broken_views,
                                                          bool native_bgr);

[[nodiscard]] bool IsLayerStrideCompatible(const ImageInfo& lhs, const ImageInfo& rhs);

[[nodiscard]] std::optional<SubresourceBase> FindSubresource(const ImageInfo& candidate,
                                                             const ImageBase& image,
                                                             GPUVAddr candidate_addr,
                                                             RelaxedOptions options,
                                                             bool broken_views, bool native_bgr);

[[nodiscard]] bool IsSubresource(const ImageInfo& candidate, const ImageBase& image,
                                 GPUVAddr candidate_addr, RelaxedOptions options, bool broken_views,
                                 bool native_bgr);

void DeduceBlitImages(ImageInfo& dst_info, ImageInfo& src_info, const ImageBase* dst,
                      const ImageBase* src);

[[nodiscard]] u32 MapSizeBytes(const ImageBase& image);

} // namespace VideoCommon
