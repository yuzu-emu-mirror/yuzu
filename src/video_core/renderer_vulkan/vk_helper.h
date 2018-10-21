// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>
#include "common/common_types.h"

namespace Vulkan {

std::optional<u32> FindMemoryType(vk::PhysicalDevice device, u32 type_filter,
                                  vk::MemoryPropertyFlags properties);

void SetImageLayout(vk::CommandBuffer cmdbuf, vk::Image image, vk::ImageLayout old_image_layout,
                    vk::ImageLayout new_image_layout, vk::ImageSubresourceRange subresource_range,
                    vk::PipelineStageFlags src_stage_mask, vk::PipelineStageFlags dst_stage_mask,
                    u32 src_family = VK_QUEUE_FAMILY_IGNORED,
                    u32 dst_family = VK_QUEUE_FAMILY_IGNORED);

void SetImageLayout(vk::CommandBuffer cmdbuf, vk::Image image, vk::ImageAspectFlags aspect_mask,
                    vk::ImageLayout old_image_layout, vk::ImageLayout new_image_layout,
                    vk::PipelineStageFlags src_stage_mask, vk::PipelineStageFlags dst_stage_mask,
                    u32 src_family = VK_QUEUE_FAMILY_IGNORED,
                    u32 dst_family = VK_QUEUE_FAMILY_IGNORED);

} // namespace Vulkan