// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <optional>
#include <vulkan/vulkan.hpp>
#include "video_core/renderer_vulkan/vk_helper.h"

namespace Vulkan {

std::optional<u32> FindMemoryType(vk::PhysicalDevice device, u32 type_filter,
                                  vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties props{device.getMemoryProperties()};
    for (u32 i = 0; i < props.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) &&
            (props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    // Couldn't find a suitable memory type
    return {};
}

void SetImageLayout(vk::CommandBuffer cmdbuf, vk::Image image, vk::ImageLayout old_image_layout,
                    vk::ImageLayout new_image_layout, vk::ImageSubresourceRange subresource_range,
                    vk::PipelineStageFlags src_stage_mask, vk::PipelineStageFlags dst_stage_mask,
                    u32 src_family, u32 dst_family) {
    vk::ImageMemoryBarrier barrier({}, {}, old_image_layout, new_image_layout, src_family,
                                   dst_family, image, subresource_range);

    switch (old_image_layout) {
    case vk::ImageLayout::eUndefined:
        barrier.srcAccessMask = {};
        break;
    case vk::ImageLayout::ePreinitialized:
        barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
        break;
    case vk::ImageLayout::eColorAttachmentOptimal:
        barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        break;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;
    case vk::ImageLayout::eTransferSrcOptimal:
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        break;
    case vk::ImageLayout::eTransferDstOptimal:
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        break;
    default:
        break;
    }

    switch (new_image_layout) {
    case vk::ImageLayout::eTransferDstOptimal:
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;
    case vk::ImageLayout::eTransferSrcOptimal:
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        break;
    case vk::ImageLayout::eColorAttachmentOptimal:
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        break;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        barrier.dstAccessMask |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        if (barrier.srcAccessMask == static_cast<vk::AccessFlagBits>(0)) {
            barrier.srcAccessMask =
                vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
        }
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        break;
    default:
        break;
    }

    cmdbuf.pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, {}, {barrier});
}

void SetImageLayout(vk::CommandBuffer cmdbuf, vk::Image image, vk::ImageAspectFlags aspect_mask,
                    vk::ImageLayout old_image_layout, vk::ImageLayout new_image_layout,
                    vk::PipelineStageFlags src_stage_mask, vk::PipelineStageFlags dst_stage_mask,
                    u32 src_family, u32 dst_family) {
    vk::ImageSubresourceRange subresource_range(aspect_mask, 0, 1, 0, 1);
    SetImageLayout(cmdbuf, image, old_image_layout, new_image_layout, subresource_range,
                   src_stage_mask, dst_stage_mask, src_family, dst_family);
}

} // namespace Vulkan