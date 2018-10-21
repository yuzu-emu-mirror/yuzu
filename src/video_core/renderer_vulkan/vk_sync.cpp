// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vulkan/vulkan.hpp>
#include "common/assert.h"
#include "common/logging/log.h"
#include "video_core/renderer_vulkan/vk_sync.h"

namespace Vulkan {

VulkanSync::VulkanSync(vk::Device& device, vk::Queue& queue, const u32& queue_family_index)
    : device(device), queue(queue) {

    calls.reserve(1024);
    CreateFreshCall();

    const vk::CommandPoolCreateInfo command_pool_ci(vk::CommandPoolCreateFlagBits::eTransient,
                                                    queue_family_index);
    one_shot_pool = device.createCommandPoolUnique(command_pool_ci);
}

VulkanSync::~VulkanSync() {
    FreeUnusedMemory();
}

void VulkanSync::AddCommand(vk::CommandBuffer cmdbuf, vk::CommandPool pool) {
    current_call->commands.push_back(cmdbuf);
    current_call->pools.push_back(pool);
}

vk::CommandBuffer VulkanSync::BeginRecord() {
    vk::CommandBufferAllocateInfo cmdbuf_ai(*one_shot_pool, vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer cmdbuf = device.allocateCommandBuffers(cmdbuf_ai)[0];

    cmdbuf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    return cmdbuf;
}

void VulkanSync::EndRecord(vk::CommandBuffer cmdbuf) {
    cmdbuf.end();
    AddCommand(cmdbuf, *one_shot_pool);
}

void VulkanSync::Execute(vk::Fence fence) {
    if (device.createSemaphore(&vk::SemaphoreCreateInfo(), nullptr, &current_call->semaphore) !=
        vk::Result::eSuccess) {
        LOG_CRITICAL(Render_Vulkan, "Vulkan failed to create a semaphore!");
        UNREACHABLE();
    }

    if (fence) {
        current_call->fence = fence;
        current_call->fence_owned = false;
    } else {
        current_call->fence = device.createFence({vk::FenceCreateFlags{}});
        current_call->fence_owned = true;
    }

    vk::SubmitInfo submit_info(0, nullptr, nullptr, static_cast<u32>(current_call->commands.size()),
                               current_call->commands.data(), 1, &current_call->semaphore);
    if (wait_semaphore) {
        // TODO(Rodrigo): This could be optimized with an extra argument.
        vk::PipelineStageFlags stage_flags = vk::PipelineStageFlagBits::eAllCommands;

        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &wait_semaphore;
        submit_info.pWaitDstStageMask = &stage_flags;
    }
    queue.submit({submit_info}, current_call->fence);

    wait_semaphore = current_call->semaphore;
    calls.push_back(std::move(current_call));
    CreateFreshCall();
}

vk::Semaphore VulkanSync::QuerySemaphore() {
    vk::Semaphore semaphore = wait_semaphore;
    wait_semaphore = vk::Semaphore(nullptr);
    return semaphore;
}

void VulkanSync::DestroyCommandBuffers() {
    constexpr u32 retries = 4;
    for (u32 i = 0; i < retries; ++i) {
        FreeUnusedMemory();
        if (calls.empty()) {
            return;
        }
    }
    // After some retries, wait for device idle and free all buffers
    device.waitIdle();
    FreeUnusedMemory();
    ASSERT(calls.empty());
}

void VulkanSync::FreeUnusedMemory() {
    auto it = calls.begin();
    while (it != calls.end()) {
        const Call* call = it->get();

        switch (device.getFenceStatus(call->fence)) {
        case vk::Result::eSuccess:
            device.destroy(call->semaphore);
            if (call->fence_owned) {
                device.destroy(call->fence);
            }
            for (std::size_t i = 0; i < call->commands.size(); i++) {
                if (vk::CommandPool pool = call->pools[i]; pool) {
                    device.freeCommandBuffers(pool, {call->commands[i]});
                }
            }
            it = calls.erase(it);
            break;
        case vk::Result::eNotReady:
            it++;
            break;
        default:
            it++;
            LOG_CRITICAL(Render_Vulkan, "Failed to get a fence status!");
            UNREACHABLE();
        }
    }
}

void VulkanSync::CreateFreshCall() {
    current_call = std::make_unique<Call>();
}

} // namespace Vulkan
