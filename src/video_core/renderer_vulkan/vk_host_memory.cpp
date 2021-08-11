// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

// Originally from
// https://github.com/google/vulkan_test_applications/blob/74e3a9790fb38303cd1646bbc098173fbb9200fa/application_sandbox/external_memory_host/main.cpp
// Copyright 2020 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/logging/log.h"
#include "video_core/renderer_vulkan/vk_host_memory.h"
#include "video_core/renderer_vulkan/vk_staging_buffer_pool.h"

namespace Vulkan {

namespace {
const VkBufferCreateInfo BUFFER_CREATE_INFO = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .size = Core::DramMemoryMap::GiB,
    .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = nullptr,
};
} // namespace

VulkanHostMemory::VulkanHostMemory(Core::DeviceMemory& memory, Device& device) {
    VkImportMemoryHostPointerInfoEXT import_memory_info{
        .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT,
        .pNext = nullptr,
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT,
        .pHostPointer = nullptr,
    };
    VkMemoryAllocateInfo allocate_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &import_memory_info,
        .allocationSize = Core::DramMemoryMap::GiB,
        .memoryTypeIndex = 0,
    };
    const auto& logical = device.GetLogical();
    const auto memory_properties = device.GetPhysical().GetMemoryProperties();
    auto host = memory.buffer.BackingBasePointer();
    for (auto& page : pages) {
        page.second = logical.CreateBuffer(BUFFER_CREATE_INFO);
        auto requirements = logical.GetBufferMemoryRequirements(*page.second, nullptr);
        if (requirements.size != Core::DramMemoryMap::GiB) {
            LOG_CRITICAL(Render_Vulkan, "Unexpected required size {}", requirements.size);
            abort();
        }
        if (requirements.alignment > 4096) {
            LOG_CRITICAL(Render_Vulkan, "Unexpected required alignment {}", requirements.alignment);
            abort();
        }
        u32 host_pointer_memory_type_bits = logical.GetMemoryHostPointerProperties(host);
        import_memory_info.pHostPointer = host;
        u32 memory_type_bits = requirements.memoryTypeBits & host_pointer_memory_type_bits;
        if (!memory_type_bits) {
            LOG_CRITICAL(
                Render_Vulkan,
                "Buffer memory bits({}) are not compatible with host pointer memory type bits ({})",
                requirements.memoryTypeBits, host_pointer_memory_type_bits);
            abort();
        }
        allocate_info.memoryTypeIndex =
            FindMemoryTypeIndex(memory_properties, memory_type_bits, false);
        page.first = logical.AllocateMemory(allocate_info);
        page.second.BindMemory(*page.first, 0);
        host += Core::DramMemoryMap::GiB;
    }
}

void VulkanHostMemory::BindPage(VKUpdateDescriptorQueue& update_descriptor_queue, u32 page) {
    if (page >= Core::DramMemoryMap::GiBs) {
        abort();
    }
//    page = 0;
    std::pair<vk::DeviceMemory, vk::Buffer>& pair = pages[page];
    [[maybe_unused]] auto mapping = pair.first.Map(0, Core::DramMemoryMap::GiB);
//    mapping[0] = 0xFF;
//    mapping[1] = 0xFF;
//    mapping[2] = 0xFF;
//    mapping[3] = 0xFF;
    pair.first.Unmap();
    update_descriptor_queue.AddBuffer(*pair.second, 0, Core::DramMemoryMap::GiB);
}

} // namespace Vulkan
