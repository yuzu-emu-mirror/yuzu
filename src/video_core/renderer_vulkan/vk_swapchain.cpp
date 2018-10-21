// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <vulkan/vulkan.hpp>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/frontend/framebuffer_layout.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"
#include "video_core/renderer_vulkan/vk_swapchain.h"

namespace Vulkan {

static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& formats) {
    if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined) {
        return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
    }

    const auto& found = std::find_if(formats.begin(), formats.end(), [](const auto& format) {
        return format.format == vk::Format::eB8G8R8A8Unorm &&
               format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    return found != formats.end() ? *found : formats[0];
}

static vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& modes) {
    // Mailbox doesn't lock the application like fifo (vsync), prefer it
    const auto& found = std::find_if(modes.begin(), modes.end(), [](const auto& mode) {
        return mode == vk::PresentModeKHR::eMailbox;
    });
    return found != modes.end() ? *found : vk::PresentModeKHR::eFifo;
}

static vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, u32 width,
                                     u32 height) {
    if (capabilities.currentExtent.width != UndefinedSize) {
        return capabilities.currentExtent;
    }
    vk::Extent2D extent = {width, height};
    extent.width = std::max(capabilities.minImageExtent.width,
                            std::min(capabilities.maxImageExtent.width, extent.width));
    extent.height = std::max(capabilities.minImageExtent.height,
                             std::min(capabilities.maxImageExtent.height, extent.height));
    return extent;
}

VulkanSwapchain::VulkanSwapchain(vk::SurfaceKHR& surface, vk::PhysicalDevice& physical_device,
                                 vk::Device& device, const u32& graphics_family,
                                 const u32& present_family)
    : surface(surface), physical_device(physical_device), device(device),
      graphics_family(graphics_family), present_family(present_family) {}

VulkanSwapchain::~VulkanSwapchain() {
    Destroy();
}

void VulkanSwapchain::Create(u32 width, u32 height) {
    const vk::SurfaceCapabilitiesKHR capabilities{
        physical_device.getSurfaceCapabilitiesKHR(surface)};
    if (capabilities.maxImageExtent.width == 0 || capabilities.maxImageExtent.height == 0) {
        return;
    }

    device.waitIdle();
    Destroy();

    CreateSwapchain(width, height, capabilities);
    CreateImageViews();
    CreateRenderPass();
    CreateFramebuffers();
    CreateFences();
}

u32 VulkanSwapchain::AcquireNextImage(vk::Semaphore present_complete) {
    u32 image_index{};
    vk::Result result{
        device.acquireNextImageKHR(*handle, WaitTimeout, present_complete, {}, &image_index)};
    if (result == vk::Result::eErrorOutOfDateKHR) {
        if (current_width > 0 && current_height > 0) {
            Create(current_width, current_height);
        }
    } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        LOG_CRITICAL(Render_Vulkan, "Failed to acquire swapchain image!");
        UNREACHABLE();
    }
    device.waitForFences(1, &fences[image_index].get(), false, WaitTimeout);
    device.resetFences(1, &fences[image_index].get());

    return image_index;
}

void VulkanSwapchain::QueuePresent(vk::Queue queue, u32 image_index,
                                   vk::Semaphore present_semaphore,
                                   vk::Semaphore render_semaphore) {
    std::array<vk::Semaphore, 2> semaphores{present_semaphore, render_semaphore};
    const u32 wait_semaphore_count{render_semaphore ? 2u : 1u};

    const vk::PresentInfoKHR present_info(wait_semaphore_count, semaphores.data(), 1, &handle.get(),
                                          &image_index, {});

    switch (queue.presentKHR(&present_info)) {
    case vk::Result::eErrorOutOfDateKHR:
        if (current_width > 0 && current_height > 0) {
            Create(current_width, current_height);
        }
        break;
    case vk::Result::eSuccess:
        break;
    default:
        LOG_CRITICAL(Render_Vulkan, "Vulkan failed to present swapchain!");
        UNREACHABLE();
    }
}

bool VulkanSwapchain::HasFramebufferChanged(const Layout::FramebufferLayout& framebuffer) const {
    // TODO(Rodrigo): Handle framebuffer pixel format changes
    return framebuffer.width != current_width || framebuffer.height != current_height;
}

const vk::Extent2D& VulkanSwapchain::GetSize() const {
    return extent;
}

const vk::Image& VulkanSwapchain::GetImage(std::size_t image_index) const {
    return images[image_index];
}

const vk::Fence& VulkanSwapchain::GetFence(std::size_t image_index) const {
    return *fences[image_index];
}

const vk::RenderPass& VulkanSwapchain::GetRenderPass() const {
    return *renderpass;
}

void VulkanSwapchain::CreateSwapchain(u32 width, u32 height,
                                      const vk::SurfaceCapabilitiesKHR& capabilities) {
    std::vector<vk::SurfaceFormatKHR> formats{physical_device.getSurfaceFormatsKHR(surface)};

    std::vector<vk::PresentModeKHR> present_modes{
        physical_device.getSurfacePresentModesKHR(surface)};

    vk::SurfaceFormatKHR surface_format{ChooseSwapSurfaceFormat(formats)};
    vk::PresentModeKHR present_mode{ChooseSwapPresentMode(present_modes)};
    extent = ChooseSwapExtent(capabilities, width, height);

    current_width = extent.width;
    current_height = extent.height;

    u32 requested_image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && requested_image_count > capabilities.maxImageCount) {
        requested_image_count = capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapchain_ci(
        {}, surface, requested_image_count, surface_format.format, surface_format.colorSpace,
        extent, 1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
        {}, {}, {}, capabilities.currentTransform, vk::CompositeAlphaFlagBitsKHR::eOpaque,
        present_mode, false, {});

    std::array<u32, 2> queue_indices{graphics_family, present_family};
    if (graphics_family != present_family) {
        swapchain_ci.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchain_ci.queueFamilyIndexCount = static_cast<u32>(queue_indices.size());
        swapchain_ci.pQueueFamilyIndices = queue_indices.data();
    } else {
        swapchain_ci.imageSharingMode = vk::SharingMode::eExclusive;
    }
    handle = device.createSwapchainKHRUnique(swapchain_ci);

    images = device.getSwapchainImagesKHR(*handle);
    image_count = static_cast<u32>(images.size());
    image_format = surface_format.format;
}

void VulkanSwapchain::CreateImageViews() {
    image_views.resize(image_count);
    for (u32 i = 0; i < image_count; i++) {
        vk::ImageViewCreateInfo image_view_ci({}, images[i], vk::ImageViewType::e2D, image_format,
                                              {}, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        image_views[i] = device.createImageViewUnique(image_view_ci);
    }
}

void VulkanSwapchain::CreateRenderPass() {
    vk::AttachmentDescription color_attachment(
        {}, image_format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference color_attachment_ref(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass_description;
    subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &color_attachment_ref;

    vk::SubpassDependency dependency(
        VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput, {},
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, {});

    vk::RenderPassCreateInfo renderpass_ci({}, 1, &color_attachment, 1, &subpass_description, 1,
                                           &dependency);
    renderpass = device.createRenderPassUnique(renderpass_ci);
}

void VulkanSwapchain::CreateFramebuffers() {
    framebuffers.resize(image_count);
    for (u32 i = 0; i < image_count; i++) {
        vk::ImageView image_view{*image_views[i]};
        const vk::FramebufferCreateInfo framebuffer_ci({}, *renderpass, 1, &image_view,
                                                       extent.width, extent.height, 1);
        framebuffers[i] = device.createFramebufferUnique(framebuffer_ci);
    }
}

void VulkanSwapchain::CreateFences() {
    fences.resize(image_count);
    for (u32 i = 0; i < image_count; i++) {
        const vk::FenceCreateInfo fence_ci(vk::FenceCreateFlagBits::eSignaled);
        fences[i] = device.createFenceUnique(fence_ci);
    }
}

void VulkanSwapchain::Destroy() {
    fences.clear();
    framebuffers.clear();
    renderpass.reset();
    image_views.clear();
    handle.reset();
}

} // namespace Vulkan