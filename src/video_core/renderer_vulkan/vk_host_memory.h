// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/device_memory.h"
#include "video_core/renderer_vulkan/vk_update_descriptor.h"
#include "video_core/vulkan_common/vulkan_device.h"

#pragma once

namespace Vulkan {

class VulkanHostMemory {
public:
    explicit VulkanHostMemory(Core::DeviceMemory& memory, Device& device);

    void BindPage(VKUpdateDescriptorQueue& update_descriptor_queue, u32 page);

private:
    std::array<std::pair<vk::DeviceMemory, vk::Buffer>, Core::DramMemoryMap::GiBs> pages;
};

} // namespace Vulkan
