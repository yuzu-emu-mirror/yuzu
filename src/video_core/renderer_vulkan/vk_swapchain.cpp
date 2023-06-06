// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <limits>
#include <vector>
#include <fmt/xchar.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/polyfill_ranges.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/frontend/emu_window.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_swapchain.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"
#include "vulkan/vulkan_core.h"

namespace Vulkan {

namespace {

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(vk::Span<VkSurfaceFormatKHR> formats) {
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        VkSurfaceFormatKHR format;
        format.format = VK_FORMAT_B8G8R8A8_UNORM;
        format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        return format;
    }
    const auto& found = std::find_if(formats.begin(), formats.end(), [](const auto& format) {
        return format.format == VK_FORMAT_B8G8R8A8_UNORM &&
               format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });
    return found != formats.end() ? *found : formats[0];
}

static VkPresentModeKHR ChooseSwapPresentMode(bool has_imm, bool has_mailbox,
                                              bool has_fifo_relaxed) {
    // Mailbox doesn't lock the application like FIFO (vsync)
    // FIFO present mode locks the framerate to the monitor's refresh rate
    Settings::VSyncMode setting = [has_imm, has_mailbox]() {
        // Choose Mailbox or Immediate if unlocked and those modes are supported
        const auto mode = Settings::values.vsync_mode.GetValue();
        if (Settings::values.use_speed_limit.GetValue()) {
            return mode;
        }
        switch (mode) {
        case Settings::VSyncMode::FIFO:
        case Settings::VSyncMode::FIFORelaxed:
            if (has_mailbox) {
                return Settings::VSyncMode::Mailbox;
            } else if (has_imm) {
                return Settings::VSyncMode::Immediate;
            }
            [[fallthrough]];
        default:
            return mode;
        }
    }();
    if ((setting == Settings::VSyncMode::Mailbox && !has_mailbox) ||
        (setting == Settings::VSyncMode::Immediate && !has_imm) ||
        (setting == Settings::VSyncMode::FIFORelaxed && !has_fifo_relaxed)) {
        setting = Settings::VSyncMode::FIFO;
    }

    switch (setting) {
    case Settings::VSyncMode::Immediate:
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    case Settings::VSyncMode::Mailbox:
        return VK_PRESENT_MODE_MAILBOX_KHR;
    case Settings::VSyncMode::FIFO:
        return VK_PRESENT_MODE_FIFO_KHR;
    case Settings::VSyncMode::FIFORelaxed:
        return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    default:
        return VK_PRESENT_MODE_FIFO_KHR;
    }
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, u32 width, u32 height) {
    constexpr auto undefined_size{std::numeric_limits<u32>::max()};
    if (capabilities.currentExtent.width != undefined_size) {
        return capabilities.currentExtent;
    }
    VkExtent2D extent;
    extent.width = std::max(capabilities.minImageExtent.width,
                            std::min(capabilities.maxImageExtent.width, width));
    extent.height = std::max(capabilities.minImageExtent.height,
                             std::min(capabilities.maxImageExtent.height, height));
    return extent;
}

VkCompositeAlphaFlagBitsKHR ChooseAlphaFlags(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
        return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    } else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
        return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    } else {
        LOG_ERROR(Render_Vulkan, "Unknown composite alpha flags value {:#x}",
                  capabilities.supportedCompositeAlpha);
        return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }
}

} // Anonymous namespace

Swapchain::Swapchain(VkSurfaceKHR surface_, const Core::Frontend::EmuWindow& emu_window_,
                     const Device& device_, Scheduler& scheduler_, u32 width_, u32 height_,
                     bool srgb)
    : surface{surface_}, emu_window{emu_window_}, device{device_}, scheduler{scheduler_},
      use_dxgi{Settings::values.use_dxgi_swapchain.GetValue()} {
    Create(width_, height_, srgb);
#ifdef WIN32
    if (use_dxgi) {
        CreateDXGIFactory();
    }
#endif
}

Swapchain::~Swapchain() = default;

void Swapchain::Create(u32 width_, u32 height_, bool srgb) {
    is_outdated = false;
    is_suboptimal = false;
    width = width_;
    height = height_;

    const auto physical_device = device.GetPhysical();
    const auto capabilities{physical_device.GetSurfaceCapabilitiesKHR(surface)};
    if (capabilities.maxImageExtent.width == 0 || capabilities.maxImageExtent.height == 0) {
        return;
    }

#ifdef WIN32
    if (dxgi_factory && use_dxgi) {
        const UINT buffer_count = static_cast<UINT>(image_count);
        dxgi_swapchain->ResizeBuffers(buffer_count, capabilities.currentExtent.width,
                                      capabilities.currentExtent.height, DXGI_FORMAT_B8G8R8A8_UNORM,
                                      0U);
    }
#endif

    Destroy();

    CreateSwapchain(capabilities, srgb);
    CreateSemaphores();
#ifdef WIN32
    if (dxgi_factory && use_dxgi) {
        ImportDXGIImages();
    }
#endif

    resource_ticks.clear();
    resource_ticks.resize(image_count);
}

bool Swapchain::AcquireNextImage() {
#ifdef WIN32
    if (use_dxgi) {
        image_index = dxgi_swapchain->GetCurrentBackBufferIndex();
        return is_suboptimal || is_outdated;
    }
#endif
    const VkResult result = device.GetLogical().AcquireNextImageKHR(
        *swapchain, std::numeric_limits<u64>::max(), *present_semaphores[frame_index],
        VK_NULL_HANDLE, &image_index);
    switch (result) {
    case VK_SUCCESS:
        break;
    case VK_SUBOPTIMAL_KHR:
        is_suboptimal = true;
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        is_outdated = true;
        break;
    default:
        LOG_ERROR(Render_Vulkan, "vkAcquireNextImageKHR returned {}", vk::ToString(result));
        break;
    }

    scheduler.Wait(resource_ticks[image_index]);
    resource_ticks[image_index] = scheduler.CurrentTick();

    return is_suboptimal || is_outdated;
}

void Swapchain::Present(VkSemaphore render_semaphore) {
#ifdef WIN32
    if (use_dxgi) {
        return PresentDXGI(render_semaphore);
    }
#endif
    const auto present_queue{device.GetPresentQueue()};
    const VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = render_semaphore ? 1U : 0U,
        .pWaitSemaphores = &render_semaphore,
        .swapchainCount = 1,
        .pSwapchains = swapchain.address(),
        .pImageIndices = &image_index,
        .pResults = nullptr,
    };
    std::scoped_lock lock{scheduler.submit_mutex};
    switch (const VkResult result = present_queue.Present(present_info)) {
    case VK_SUCCESS:
        break;
    case VK_SUBOPTIMAL_KHR:
        LOG_DEBUG(Render_Vulkan, "Suboptimal swapchain");
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        is_outdated = true;
        break;
    default:
        LOG_CRITICAL(Render_Vulkan, "Failed to present with error {}", vk::ToString(result));
        break;
    }
    ++frame_index;
    if (frame_index >= image_count) {
        frame_index = 0;
    }
}

void Swapchain::CreateSwapchain(const VkSurfaceCapabilitiesKHR& capabilities, bool srgb) {
    const auto physical_device{device.GetPhysical()};
    const auto formats{physical_device.GetSurfaceFormatsKHR(surface)};
    const auto present_modes = physical_device.GetSurfacePresentModesKHR(surface);
    has_mailbox = std::find(present_modes.begin(), present_modes.end(),
                            VK_PRESENT_MODE_MAILBOX_KHR) != present_modes.end();
    has_imm = std::find(present_modes.begin(), present_modes.end(),
                        VK_PRESENT_MODE_IMMEDIATE_KHR) != present_modes.end();
    has_fifo_relaxed = std::find(present_modes.begin(), present_modes.end(),
                                 VK_PRESENT_MODE_FIFO_RELAXED_KHR) != present_modes.end();
    current_srgb = srgb;
    image_view_format = srgb ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM;

    const VkCompositeAlphaFlagBitsKHR alpha_flags{ChooseAlphaFlags(capabilities)};
    surface_format = ChooseSwapSurfaceFormat(formats);
    present_mode = ChooseSwapPresentMode(has_imm, has_mailbox, has_fifo_relaxed);

    u32 requested_image_count{capabilities.minImageCount + 1};
    // Ensure Triple buffering if possible.
    if (capabilities.maxImageCount > 0) {
        if (requested_image_count > capabilities.maxImageCount) {
            requested_image_count = capabilities.maxImageCount;
        } else {
            requested_image_count =
                std::max(requested_image_count, std::min(3U, capabilities.maxImageCount));
        }
    } else {
        requested_image_count = std::max(requested_image_count, 3U);
    }
    VkSwapchainCreateInfoKHR swapchain_ci{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = surface,
        .minImageCount = requested_image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = {},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = alpha_flags,
        .presentMode = present_mode,
        .clipped = VK_FALSE,
        .oldSwapchain = nullptr,
    };
    const u32 graphics_family{device.GetGraphicsFamily()};
    const u32 present_family{device.GetPresentFamily()};
    const std::array<u32, 2> queue_indices{graphics_family, present_family};
    if (graphics_family != present_family) {
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_ci.queueFamilyIndexCount = static_cast<u32>(queue_indices.size());
        swapchain_ci.pQueueFamilyIndices = queue_indices.data();
    }
    static constexpr std::array view_formats{VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SRGB};
    VkImageFormatListCreateInfo format_list{
        .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR,
        .pNext = nullptr,
        .viewFormatCount = static_cast<u32>(view_formats.size()),
        .pViewFormats = view_formats.data(),
    };
    if (device.IsKhrSwapchainMutableFormatEnabled()) {
        format_list.pNext = std::exchange(swapchain_ci.pNext, &format_list);
        swapchain_ci.flags |= VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
    }
    // Request the size again to reduce the possibility of a TOCTOU race condition.
    const auto updated_capabilities = physical_device.GetSurfaceCapabilitiesKHR(surface);
    swapchain_ci.imageExtent = ChooseSwapExtent(updated_capabilities, width, height);
    // Don't add code within this and the swapchain creation.
    swapchain = device.GetLogical().CreateSwapchainKHR(swapchain_ci);

    extent = swapchain_ci.imageExtent;
    images = swapchain.GetImages();
    image_count = static_cast<u32>(images.size());
}

void Swapchain::CreateSemaphores() {
    present_semaphores.resize(image_count);
    std::ranges::generate(present_semaphores,
                          [this] { return device.GetLogical().CreateSemaphore(); });
    render_semaphores.resize(image_count);
    std::ranges::generate(render_semaphores,
                          [this] { return device.GetLogical().CreateSemaphore(); });
}

void Swapchain::Destroy() {
    frame_index = 0;
    swapchain.reset();
#ifdef WIN32
    if (!use_dxgi) {
        return;
    }
    for (HANDLE handle : shared_handles) {
        CloseHandle(handle);
    }
    shared_handles.clear();
    present_semaphores.clear();
    imported_memories.clear();
    dx_vk_images.clear();
#endif
}

bool Swapchain::NeedsPresentModeUpdate() const {
    const auto requested_mode = ChooseSwapPresentMode(has_imm, has_mailbox, has_fifo_relaxed);
    return present_mode != requested_mode;
}

#ifdef WIN32
void Swapchain::CreateDXGIFactory() {
    UINT create_factory_flags = 0;
    CreateDXGIFactory2(create_factory_flags, IID_PPV_ARGS(&dxgi_factory));

    VkPhysicalDeviceIDProperties device_id_props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR,
        .pNext = nullptr,
    };
    VkPhysicalDeviceProperties2 device_props_2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR,
        .pNext = &device_id_props,
    };

    device.GetPhysical().GetProperties2(device_props_2);
    if (!device_id_props.deviceLUIDValid) {
        LOG_ERROR(Render_Vulkan, "Device LUID not valid, DXGI interop not possible");
        return;
    }

    const LUID* pluid = reinterpret_cast<LUID*>(&device_id_props.deviceLUID);
    dxgi_factory->EnumAdapterByLuid(*pluid, IID_PPV_ARGS(&dxgi_adapter));
    D3D12CreateDevice(dxgi_adapter.Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&dx_device));

    const UINT node_count = dx_device->GetNodeCount();
    const UINT node_mask = node_count <= 1 ? 0 : device_id_props.deviceNodeMask;

    const D3D12_COMMAND_QUEUE_DESC dxQDesc = {D3D12_COMMAND_LIST_TYPE_DIRECT,
                                              D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
                                              D3D12_COMMAND_QUEUE_FLAG_NONE, node_mask};
    dx_device->CreateCommandQueue(&dxQDesc, IID_PPV_ARGS(&dx_command_queue));

    const UINT buffer_count = static_cast<UINT>(image_count);
    const DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {
        .Width = extent.width,
        .Height = extent.height,
        .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
        .Stereo = FALSE,
        .SampleDesc = {1, 0},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = buffer_count,
        .Scaling = DXGI_SCALING_NONE,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
        .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
        .Flags = 0,
    };

    const HWND hWnd = static_cast<HWND>(emu_window.GetWindowInfo().render_surface);
    dxgi_factory->CreateSwapChainForHwnd(dx_command_queue.Get(), hWnd, &swapchain_desc, nullptr,
                                         nullptr, &dxgi_swapchain1);
    dxgi_factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
    dxgi_swapchain1.As(&dxgi_swapchain);

    present_fence = device.GetLogical().CreateFence({
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    });
}

void Swapchain::ImportDXGIImages() {
    if (!dxgi_factory) {
        return;
    }

    const auto& dld = device.GetLogical();
    std::vector<ComPtr<ID3D12Resource>> dx_images(image_count);

    images.clear();
    images.resize(image_count);
    imported_memories.resize(image_count);
    shared_handles.resize(image_count);
    dx_vk_images.resize(image_count);

    for (UINT i = 0; i < dx_images.size(); i++) {
        dxgi_swapchain->GetBuffer(i, IID_PPV_ARGS(&dx_images[i]));
    }

    for (size_t i = 0; i < dx_images.size(); i++) {
        const auto& dxImage = dx_images[i];
        const auto dxImageDesc = dxImage->GetDesc();

        D3D12_HEAP_PROPERTIES dx_image_heap;
        D3D12_HEAP_FLAGS dx_image_heap_flags;
        dxImage->GetHeapProperties(&dx_image_heap, &dx_image_heap_flags);

        const VkExternalMemoryImageCreateInfoKHR eii = {
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT_KHR};

        const VkImageCreateInfo image_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = &eii,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .extent =
                {
                    .width = static_cast<u32>(dxImageDesc.Width),
                    .height = static_cast<u32>(dxImageDesc.Height),
                    .depth = 1,
                },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        dx_vk_images[i] = dld.CreateImage(image_ci);
        images[i] = *dx_vk_images[i];

        const std::wstring handle_name = fmt::format(L"DXImage{}", i);
        dx_device->CreateSharedHandle(dxImage.Get(), NULL, GENERIC_ALL, handle_name.data(),
                                      &shared_handles[i]);

        static constexpr u32 BAD_BITS = 0xcdcdcdcd;

        VkMemoryWin32HandlePropertiesKHR win32_memory_props = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR,
            .pNext = nullptr,
            .memoryTypeBits = BAD_BITS,
        };
        dld.GetMemoryWin32HandlePropertiesKHR(VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT,
                                              shared_handles[i], win32_memory_props);

        const auto requirements = dld.GetImageMemoryRequirements(images[i]);
        if (win32_memory_props.memoryTypeBits == BAD_BITS) [[unlikely]] {
            win32_memory_props.memoryTypeBits = requirements.memoryTypeBits;
        } else {
            win32_memory_props.memoryTypeBits &= requirements.memoryTypeBits;
        }

        // Find the memory type the image should be placed in.
        int mem_type_index = -1;
        const auto properties = device.GetPhysical().GetMemoryProperties().memoryProperties;
        for (u32 im = 0; im < properties.memoryTypeCount; ++im) {
            const u32 current_bit = 0x1 << im;
            if (win32_memory_props.memoryTypeBits == current_bit) {
                if (properties.memoryTypes[im].propertyFlags &
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                    mem_type_index = im;
                    break;
                }
            }
        }
        ASSERT_MSG(mem_type_index != -1, "Device local import memory not found!");

        // DX12 resources need to be dedicated per the vulkan spec.
        const VkMemoryDedicatedAllocateInfoKHR dii = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
            .pNext = nullptr,
            .image = images[i],
            .buffer = VK_NULL_HANDLE,
        };

        const VkImportMemoryWin32HandleInfoKHR imi = {
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
            .pNext = &dii,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT_KHR,
            .handle = shared_handles[i],
            .name = nullptr};

        const VkMemoryAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                                 .pNext = &imi,
                                                 .allocationSize = requirements.size,
                                                 .memoryTypeIndex =
                                                     static_cast<u32>(mem_type_index)};

        // Bind imported memory to created image
        imported_memories[i] = dld.AllocateMemory(alloc_info);
        dx_vk_images[i].BindMemory(*imported_memories[i], 0);
    }
}

void Swapchain::PresentDXGI(VkSemaphore render_semaphore) {
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    const VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &render_semaphore,
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 0,
        .pCommandBuffers = nullptr,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };
    const auto present_queue{device.GetPresentQueue()};
    present_queue.Submit(submit, *present_fence);

    // Wait for the generated image to be rendered before calling present from D3D.
    // TODO: Is there any better way to sync? Call vulkan semaphore from D3D?
    present_fence.Wait();
    present_fence.Reset();

    const DXGI_PRESENT_PARAMETERS present_params = {};
    dxgi_swapchain->Present1(1, 0, &present_params);

    ++frame_index;
    if (frame_index >= image_count) {
        frame_index = 0;
    }
}
#endif

} // namespace Vulkan
