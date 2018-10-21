// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <limits>
#include <set>

#include <vulkan/vulkan.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/frontend/emu_window.h"
#include "core/memory.h"
#include "core/perf_stats.h"
#include "core/settings.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"
#include "video_core/renderer_vulkan/vk_helper.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"
#include "video_core/renderer_vulkan/vk_swapchain.h"
#include "video_core/renderer_vulkan/vk_sync.h"
#include "video_core/utils.h"

namespace Vulkan {

RendererVulkan::RendererVulkan(Core::Frontend::EmuWindow& window) : RendererBase(window) {}

RendererVulkan::~RendererVulkan() {
    ShutDown();
}

void RendererVulkan::SwapBuffers(
    std::optional<std::reference_wrapper<const Tegra::FramebufferConfig>> framebuffer) {

    Core::System::GetInstance().GetPerfStats().EndSystemFrame();

    const auto& layout = render_window.GetFramebufferLayout();
    if (framebuffer && layout.width > 0 && layout.height > 0 && render_window.IsShown()) {
        if (swapchain->HasFramebufferChanged(layout)) {
            sync->DestroyCommandBuffers();
            swapchain->Create(layout.width, layout.height);
        }

        const u32 image_index = swapchain->AcquireNextImage(present_semaphore);
        DrawScreen(*framebuffer, image_index);

        vk::Semaphore render_semaphore = sync->QuerySemaphore();
        swapchain->QueuePresent(present_queue, image_index, present_semaphore, render_semaphore);

        render_window.SwapBuffers();
    }

    render_window.PollEvents();

    Core::System::GetInstance().FrameLimiter().DoFrameLimiting(CoreTiming::GetGlobalTimeUs());
    Core::System::GetInstance().GetPerfStats().BeginSystemFrame();
}

bool RendererVulkan::Init() {
    CreateRasterizer();
    return InitVulkanObjects();
}

void RendererVulkan::ShutDown() {
    device.waitIdle();

    // Always destroy sync before swapchain because sync will countain fences used by swapchain.
    sync.reset();
    swapchain.reset();

    device.destroy(screen_info.staging_image);
    device.free(screen_info.staging_memory);

    device.destroy(present_semaphore);
    device.destroy();
}

void RendererVulkan::CreateRasterizer() {
    if (rasterizer) {
        return;
    }
    rasterizer = std::make_unique<RasterizerVulkan>(render_window, screen_info);
}

bool RendererVulkan::InitVulkanObjects() {
    render_window.RetrieveVulkanHandlers(reinterpret_cast<void**>(&instance),
                                         reinterpret_cast<void**>(&surface));
    if (!PickPhysicalDevice()) {
        return false;
    }
    if (!CreateLogicalDevice()) {
        return false;
    }
    const auto& framebuffer = render_window.GetFramebufferLayout();
    swapchain = std::make_unique<VulkanSwapchain>(surface, physical_device, device,
                                                  graphics_family_index, present_family_index);
    swapchain->Create(framebuffer.width, framebuffer.height);

    sync = std::make_unique<VulkanSync>(device, graphics_queue, graphics_family_index);

    if (device.createSemaphore(&vk::SemaphoreCreateInfo(), nullptr, &present_semaphore) !=
        vk::Result::eSuccess) {
        return false;
    }

    return true;
}

bool RendererVulkan::PickPhysicalDevice() {
    u32 device_count{};
    if (instance.enumeratePhysicalDevices(&device_count, nullptr) != vk::Result::eSuccess ||
        device_count == 0) {
        LOG_ERROR(Render_Vulkan, "No Vulkan devices found!");
        return false;
    }
    std::vector<vk::PhysicalDevice> devices(device_count);
    instance.enumeratePhysicalDevices(&device_count, devices.data());

    s32 device_index = Settings::values.vulkan_device;
    if (device_index < 0 || device_index >= static_cast<s32>(device_count)) {
        LOG_ERROR(Render_Vulkan, "Invalid device index {}!", device_index);
        return false;
    }
    physical_device = devices[device_index];

    if (!IsDeviceSuitable(physical_device)) {
        LOG_ERROR(Render_Vulkan, "Device {} is not suitable!", device_index);
        return false;
    }

    std::vector<vk::QueueFamilyProperties> queue_families{
        physical_device.getQueueFamilyProperties()};
    int i{};
    for (const auto& queue_family : queue_families) {
        if (queue_family.queueCount > 0) {
            if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
                graphics_family_index = i;
            }
            if (physical_device.getSurfaceSupportKHR(i, surface)) {
                present_family_index = i;
            }
        }
        i++;
    }

    if (graphics_family_index == UndefinedFamily || present_family_index == UndefinedFamily) {
        LOG_ERROR(Render_Vulkan, "Device has not enough queues!");
        return false;
    }

    LOG_INFO(Render_Vulkan, "{}", physical_device.getProperties().deviceName);
    return true;
}

bool RendererVulkan::CreateLogicalDevice() {
    const float queue_priorities{1.f};
    std::vector<vk::DeviceQueueCreateInfo> queue_cis{GetDeviceQueueCreateInfos(&queue_priorities)};

    vk::PhysicalDeviceFeatures device_features{};

    std::vector<const char*> extensions{};
    extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    vk::DeviceCreateInfo device_ci({}, static_cast<u32>(queue_cis.size()), queue_cis.data(), 0,
                                   nullptr, static_cast<u32>(extensions.size()), extensions.data(),
                                   &device_features);
    if (physical_device.createDevice(&device_ci, nullptr, &device) != vk::Result::eSuccess) {
        LOG_CRITICAL(Render_Vulkan, "Device failed to be created!");
        return false;
    }

    graphics_queue = device.getQueue(graphics_family_index, 0);
    present_queue = device.getQueue(present_family_index, 0);
    return true;
}

bool RendererVulkan::IsDeviceSuitable(vk::PhysicalDevice physical_device) const {
    // TODO(Rodrigo): Query suitability before creating logical device
    return true;
}

std::vector<vk::DeviceQueueCreateInfo> RendererVulkan::GetDeviceQueueCreateInfos(
    const float* queue_priority) const {
    std::vector<vk::DeviceQueueCreateInfo> queue_cis;
    std::set<u32> unique_queue_families = {graphics_family_index, present_family_index};

    for (u32 queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_ci{};
        queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_ci.queueFamilyIndex = queue_family;
        queue_ci.queueCount = 1;
        queue_ci.pQueuePriorities = queue_priority;
        queue_cis.push_back(queue_ci);
    }
    return queue_cis;
}

void RendererVulkan::DrawScreen(const Tegra::FramebufferConfig& framebuffer, u32 image_index) {
    const u32 bytes_per_pixel{Tegra::FramebufferConfig::BytesPerPixel(framebuffer.pixel_format)};
    const u64 size_in_bytes{framebuffer.stride * framebuffer.height * bytes_per_pixel};
    const VAddr framebuffer_addr{framebuffer.address + framebuffer.offset};
    const vk::Extent2D& framebuffer_size{swapchain->GetSize()};

    if (rasterizer->AccelerateDisplay(framebuffer, framebuffer_addr, framebuffer.stride)) {
        return;
    }

    const bool recreate{!screen_info.staging_image || framebuffer.width != screen_info.width ||
                        framebuffer.height != screen_info.height};
    if (recreate) {
        if (screen_info.staging_image || screen_info.staging_memory) {
            ASSERT(screen_info.staging_image && screen_info.staging_memory);

            // Wait to avoid using staging memory while it's being transfered
            device.waitIdle();
            device.destroy(screen_info.staging_image);
            device.free(screen_info.staging_memory);
        }

        const vk::ImageCreateInfo image_ci(
            {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            {framebuffer.width, framebuffer.height, 1}, 1, 1, vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eLinear, vk::ImageUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::ePreinitialized);
        screen_info.staging_image = device.createImage(image_ci);

        vk::MemoryRequirements mem_reqs{
            device.getImageMemoryRequirements(screen_info.staging_image)};

        const auto memory_type = FindMemoryType(physical_device, mem_reqs.memoryTypeBits,
                                                vk::MemoryPropertyFlagBits::eHostVisible |
                                                    vk::MemoryPropertyFlagBits::eHostCoherent);
        if (!memory_type) {
            LOG_CRITICAL(Render_Vulkan, "Couldn't find a suitable memory type!");
            UNREACHABLE();
        }
        // TODO(Rodrigo): Use a memory allocator
        screen_info.staging_memory = device.allocateMemory({mem_reqs.size, *memory_type});

        device.bindImageMemory(screen_info.staging_image, screen_info.staging_memory, 0);

        screen_info.width = framebuffer.width;
        screen_info.height = framebuffer.height;
        screen_info.size_in_bytes = mem_reqs.size;
    }

    Memory::RasterizerFlushVirtualRegion(framebuffer_addr, size_in_bytes, Memory::FlushMode::Flush);

    void* data = device.mapMemory(screen_info.staging_memory, 0, screen_info.size_in_bytes, {});

    VideoCore::MortonCopyPixels128(framebuffer.width, framebuffer.height, bytes_per_pixel, 4,
                                   Memory::GetPointer(framebuffer_addr), static_cast<u8*>(data),
                                   true);
    device.unmapMemory(screen_info.staging_memory);

    // Record blitting
    vk::CommandBuffer cmdbuf{sync->BeginRecord()};

    if (recreate) {
        SetImageLayout(cmdbuf, screen_info.staging_image, vk::ImageAspectFlagBits::eColor,
                       vk::ImageLayout::ePreinitialized, vk::ImageLayout::eGeneral,
                       vk::PipelineStageFlagBits::eHost,
                       vk::PipelineStageFlagBits::eHost | vk::PipelineStageFlagBits::eTransfer);
    }
    SetImageLayout(cmdbuf, swapchain->GetImage(image_index), vk::ImageAspectFlagBits::eColor,
                   vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                   vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer);

    // TODO(Rodrigo): Use clip values
    const bool flip_y =
        framebuffer.transform_flags == Tegra::FramebufferConfig::TransformFlags::FlipV;
    const s32 y0 = flip_y ? static_cast<s32>(framebuffer_size.height) : 0;
    const s32 y1 = flip_y ? 0 : static_cast<s32>(framebuffer_size.height);
    vk::ImageSubresourceLayers subresource(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
    std::array<vk::Offset3D, 2> src_offsets, dst_offsets;
    src_offsets[0] = {0, 0, 0};
    src_offsets[1] = {static_cast<s32>(screen_info.width), static_cast<s32>(screen_info.height), 1};
    dst_offsets[0] = {0, y0, 0};
    dst_offsets[1] = {static_cast<s32>(framebuffer_size.width), y1, 1};
    const vk::ImageBlit blit(subresource, src_offsets, subresource, dst_offsets);

    cmdbuf.blitImage(screen_info.staging_image, vk::ImageLayout::eGeneral,
                     swapchain->GetImage(image_index), vk::ImageLayout::eTransferDstOptimal, {blit},
                     vk::Filter::eLinear);

    SetImageLayout(cmdbuf, swapchain->GetImage(image_index), vk::ImageAspectFlagBits::eColor,
                   vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR,
                   vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer);

    sync->EndRecord(cmdbuf);
    sync->Execute(swapchain->GetFence(image_index));
}

} // namespace Vulkan