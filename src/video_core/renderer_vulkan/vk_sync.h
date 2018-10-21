// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "common/common_types.h"

namespace Vulkan {

class VulkanSync {
public:
    explicit VulkanSync(vk::Device& device, vk::Queue& queue, const u32& queue_family_index);
    ~VulkanSync();

    /**
     * Adds a command to execution queue.
     * Passing a null handle for pool means that the command buffer has to be externally freed.
     */
    void AddCommand(vk::CommandBuffer cmdbuf, vk::CommandPool pool = vk::CommandPool(nullptr));

    vk::CommandBuffer BeginRecord();

    void EndRecord(vk::CommandBuffer cmdbuf);

    void Execute(vk::Fence fence = vk::Fence(nullptr));

    vk::Semaphore QuerySemaphore();

    void DestroyCommandBuffers();

    void FreeUnusedMemory();

private:
    struct Call {
        std::vector<vk::CommandBuffer> commands;
        std::vector<vk::CommandPool> pools;
        vk::Fence fence;
        bool fence_owned;
        vk::Semaphore semaphore;
    };

    void CreateFreshCall();

    vk::Device& device;
    vk::Queue& queue;

    std::vector<std::unique_ptr<Call>> calls;
    std::unique_ptr<Call> current_call;

    vk::UniqueCommandPool one_shot_pool;

    vk::Semaphore wait_semaphore{};
};

} // namespace Vulkan