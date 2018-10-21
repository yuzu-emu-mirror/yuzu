// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <limits>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "common/common_types.h"

namespace Layout {
struct FramebufferLayout;
}

namespace Vulkan {

class VulkanSwapchain {
public:
    explicit VulkanSwapchain(vk::SurfaceKHR& surface, vk::PhysicalDevice& physical_device,
                             vk::Device& device, const u32& graphics_family,
                             const u32& present_family);
    ~VulkanSwapchain();

    void Create(u32 width, u32 height);

    u32 AcquireNextImage(vk::Semaphore present_complete);

    void QueuePresent(vk::Queue queue, u32 image_index, vk::Semaphore present_semaphore,
                      vk::Semaphore render_semaphore);

    bool HasFramebufferChanged(const Layout::FramebufferLayout& framebuffer) const;

    const vk::Extent2D& GetSize() const;
    const vk::Image& GetImage(std::size_t image_index) const;
    const vk::Fence& GetFence(std::size_t image_index) const;
    const vk::RenderPass& GetRenderPass() const;

private:
    void CreateSwapchain(u32 width, u32 height, const vk::SurfaceCapabilitiesKHR& capabilities);
    void CreateImageViews();
    void CreateRenderPass();
    void CreateFramebuffers();
    void CreateFences();

    void Destroy();

    vk::SurfaceKHR& surface;
    vk::PhysicalDevice& physical_device;
    vk::Device& device;
    const u32& graphics_family;
    const u32& present_family;

    vk::UniqueSwapchainKHR handle;

    u32 current_width{};
    u32 current_height{};

    vk::Format image_format{};
    vk::Extent2D extent{};

    u32 image_count{};
    std::vector<vk::Image> images;
    std::vector<vk::UniqueImageView> image_views;
    std::vector<vk::UniqueFramebuffer> framebuffers;
    std::vector<vk::UniqueFence> fences;

    vk::UniqueRenderPass renderpass;
};

} // namespace Vulkan