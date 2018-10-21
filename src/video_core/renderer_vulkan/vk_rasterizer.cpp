// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "video_core/renderer_vulkan/vk_rasterizer.h"

namespace Vulkan {

RasterizerVulkan::RasterizerVulkan(Core::Frontend::EmuWindow& renderer, VulkanScreenInfo& info)
    : VideoCore::RasterizerInterface() {}

RasterizerVulkan::~RasterizerVulkan() = default;

void RasterizerVulkan::DrawArrays() {}

void RasterizerVulkan::Clear() {}

void RasterizerVulkan::FlushAll() {}

void RasterizerVulkan::FlushRegion(Tegra::GPUVAddr addr, u64 size) {}

void RasterizerVulkan::InvalidateRegion(Tegra::GPUVAddr addr, u64 size) {}

void RasterizerVulkan::FlushAndInvalidateRegion(Tegra::GPUVAddr addr, u64 size) {}

} // namespace Vulkan