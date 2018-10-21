// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "video_core/rasterizer_interface.h"

namespace Core::Frontend {
class EmuWindow;
}

namespace Vulkan {

struct VulkanScreenInfo;

class RasterizerVulkan : public VideoCore::RasterizerInterface {
public:
    explicit RasterizerVulkan(Core::Frontend::EmuWindow& renderer, VulkanScreenInfo& info);
    ~RasterizerVulkan() override;

    void DrawArrays() override;
    void Clear() override;
    void FlushAll() override;
    void FlushRegion(Tegra::GPUVAddr addr, u64 size) override;
    void InvalidateRegion(Tegra::GPUVAddr addr, u64 size) override;
    void FlushAndInvalidateRegion(Tegra::GPUVAddr addr, u64 size) override;
};

} // namespace Vulkan