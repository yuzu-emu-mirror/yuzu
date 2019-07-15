// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <glad/glad.h>
#include "common/common_types.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/staging_buffer_cache.h"

namespace OpenGL {

class Device;
class StagingBuffer;

class StagingBufferCache final : public VideoCommon::StagingBufferCache<StagingBuffer> {
public:
    explicit StagingBufferCache(const Device& device);
    ~StagingBufferCache() override;

protected:
    std::unique_ptr<StagingBuffer> CreateBuffer(std::size_t size, bool is_flush) override;

private:
    const Device& device;
};

class StagingBuffer : public NonCopyable {
public:
    virtual ~StagingBuffer() = default;

    /// Returns the base pointer passed to an OpenGL function.
    [[nodiscard]] virtual u8* GetOpenGLPointer() const = 0;

    /// Maps the staging buffer.
    [[nodiscard]] virtual u8* Map(std::size_t size) const = 0;

    /// Unmaps the staging buffer
    virtual void Unmap(std::size_t size) const = 0;

    /// Inserts a fence in the OpenGL pipeline.
    /// @param own Protects the fence from being used before it's waited, intended for flushes.
    virtual void QueueFence(bool own) = 0;

    /// Waits for a fence and releases the ownership.
    virtual void WaitFence() = 0;

    /// Discards the deferred operation and its bound fence. A fence must be queued.
    virtual void Discard() = 0;

    /// Returns true when the fence is available.
    [[nodiscard]] virtual bool IsAvailable() = 0;

    /// Binds the staging buffer handle to an OpenGL target.
    virtual void Bind(GLenum target) const = 0;
};

} // namespace OpenGL
