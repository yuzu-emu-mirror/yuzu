// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cstring>
#include <memory>

#include "core/core.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/renderer_vulkan/vk_buffer_cache.h"
#include "video_core/renderer_vulkan/vk_device.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_stream_buffer.h"
#include "video_core/renderer_vulkan/wrapper.h"

namespace Vulkan {

namespace {

constexpr VkBufferUsageFlags BUFFER_USAGE =
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

constexpr VkPipelineStageFlags UPLOAD_PIPELINE_STAGE =
    VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

constexpr VkAccessFlags UPLOAD_ACCESS_BARRIERS =
    VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT |
    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;

std::unique_ptr<VKStreamBuffer> CreateStreamBuffer(const VKDevice& device, VKScheduler& scheduler) {
    return std::make_unique<VKStreamBuffer>(device, scheduler, BUFFER_USAGE);
}

} // Anonymous namespace

CachedBuffer::CachedBuffer(const VKDevice& device, VKMemoryManager& memory_manager,
                           VKScheduler& scheduler_, VKStagingBufferPool& staging_pool_,
                           VAddr cpu_addr, std::size_t size)
    : BufferBlock{cpu_addr, size}, scheduler{scheduler_}, staging_pool{staging_pool_} {
    const VkBufferCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = static_cast<VkDeviceSize>(size),
        .usage = BUFFER_USAGE | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    buffer.handle = device.GetLogical().CreateBuffer(ci);
    buffer.commit = memory_manager.Commit(buffer.handle, false);
}

CachedBuffer::~CachedBuffer() = default;

void CachedBuffer::Upload(std::size_t offset, std::size_t size, const u8* data) {
    const Buffer& staging = staging_pool.GetUnusedBuffer(size, true);
    std::memcpy(staging.Map(size), data, size);

    scheduler.RequestOutsideRenderPassOperationContext();

    const VkBuffer handle = Handle();
    scheduler.Record([staging = staging.Handle(), handle, offset, size](vk::CommandBuffer cmdbuf) {
        cmdbuf.CopyBuffer(staging, handle, VkBufferCopy{0, offset, size});

        const VkBufferMemoryBarrier barrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = UPLOAD_ACCESS_BARRIERS,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = handle,
            .offset = offset,
            .size = size,
        };
        cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, UPLOAD_PIPELINE_STAGE, 0, {},
                               barrier, {});
    });
}

void CachedBuffer::Download(std::size_t offset, std::size_t size, u8* data) {
    const Buffer& staging = staging_pool.GetUnusedBuffer(size, true);
    scheduler.RequestOutsideRenderPassOperationContext();

    const VkBuffer handle = Handle();
    scheduler.Record([staging = *staging.handle, handle, offset, size](vk::CommandBuffer cmdbuf) {
        const VkBufferMemoryBarrier barrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = handle,
            .offset = offset,
            .size = size,
        };

        cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                               VK_PIPELINE_STAGE_TRANSFER_BIT, 0, {}, barrier, {});
        cmdbuf.CopyBuffer(handle, staging, VkBufferCopy{offset, 0, size});
    });
    scheduler.Finish();

    std::memcpy(data, staging.Map(size), size);
}

void CachedBuffer::CopyFrom(const CachedBuffer& src, std::size_t src_offset, std::size_t dst_offset,
                            std::size_t size) {
    scheduler.RequestOutsideRenderPassOperationContext();

    const VkBuffer dst_buffer = Handle();
    scheduler.Record([src_buffer = src.Handle(), dst_buffer, src_offset, dst_offset,
                      size](vk::CommandBuffer cmdbuf) {
        cmdbuf.CopyBuffer(src_buffer, dst_buffer, VkBufferCopy{src_offset, dst_offset, size});

        const std::array barriers{
            VkBufferMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = src_buffer,
                .offset = src_offset,
                .size = size,
            },
            VkBufferMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = UPLOAD_ACCESS_BARRIERS,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = dst_buffer,
                .offset = dst_offset,
                .size = size,
            },
        };
        cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, UPLOAD_PIPELINE_STAGE, 0, {},
                               barriers, {});
    });
}

VKBufferCache::VKBufferCache(VideoCore::RasterizerInterface& rasterizer,
                             Tegra::MemoryManager& gpu_memory, Core::Memory::Memory& cpu_memory,
                             const VKDevice& device_, VKMemoryManager& memory_manager_,
                             VKScheduler& scheduler_, VKStagingBufferPool& staging_pool_)
    : VideoCommon::BufferCache<CachedBuffer, VkBuffer, VKStreamBuffer>{rasterizer, gpu_memory,
                                                                       cpu_memory,
                                                                       CreateStreamBuffer(
                                                                           device_, scheduler_)},
      device{device_}, memory_manager{memory_manager_}, scheduler{scheduler_}, staging_pool{
                                                                                   staging_pool_} {}

VKBufferCache::~VKBufferCache() = default;

std::shared_ptr<CachedBuffer> VKBufferCache::CreateBlock(VAddr cpu_addr, std::size_t size) {
    return std::make_shared<CachedBuffer>(device, memory_manager, scheduler, staging_pool, cpu_addr,
                                          size);
}

VKBufferCache::BufferInfo VKBufferCache::GetEmptyBuffer(std::size_t size) {
    size = std::max(size, std::size_t(4));
    const Buffer& empty = staging_pool.GetUnusedBuffer(size, false);
    scheduler.RequestOutsideRenderPassOperationContext();
    scheduler.Record([size, buffer = empty.Handle()](vk::CommandBuffer cmdbuf) {
        cmdbuf.FillBuffer(buffer, 0, size, 0);
    });
    return {empty.Handle(), 0, 0};
}

} // namespace Vulkan
