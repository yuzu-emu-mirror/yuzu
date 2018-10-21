// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <limits>
#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "video_core/renderer_base.h"

namespace Vulkan {

constexpr auto UndefinedSize = std::numeric_limits<u32>::max();
constexpr auto UndefinedFamily = std::numeric_limits<u32>::max();

constexpr auto WaitTimeout = std::numeric_limits<u64>::max();

class VulkanSwapchain;
class VulkanSync;

struct VulkanScreenInfo {
    u32 width{};
    u32 height{};
    u64 size_in_bytes{};
    vk::Image staging_image;
    vk::DeviceMemory staging_memory;
};

class RendererVulkan : public VideoCore::RendererBase {
public:
    explicit RendererVulkan(Core::Frontend::EmuWindow& window);
    ~RendererVulkan() override;

    /// Swap buffers (render frame)
    void SwapBuffers(
        std::optional<std::reference_wrapper<const Tegra::FramebufferConfig>> framebuffer) override;

    /// Initialize the renderer
    bool Init() override;

    /// Shutdown the renderer
    void ShutDown() override;

private:
    void CreateRasterizer();
    bool InitVulkanObjects();
    bool PickPhysicalDevice();
    bool CreateLogicalDevice();

    bool IsDeviceSuitable(vk::PhysicalDevice physical_device) const;

    std::vector<vk::DeviceQueueCreateInfo> GetDeviceQueueCreateInfos(
        const float* queue_priority) const;

    void DrawScreen(const Tegra::FramebufferConfig& framebuffer, u32 image_index);

    vk::Instance instance;
    vk::SurfaceKHR surface;

    vk::PhysicalDevice physical_device;
    vk::Device device;

    u32 graphics_family_index = UndefinedFamily;
    u32 present_family_index = UndefinedFamily;

    vk::Queue graphics_queue{};
    vk::Queue present_queue{};

    VulkanScreenInfo screen_info{};

    std::unique_ptr<VulkanSwapchain> swapchain;

    std::unique_ptr<VulkanSync> sync;

    vk::Semaphore present_semaphore;
};

} // namespace Vulkan