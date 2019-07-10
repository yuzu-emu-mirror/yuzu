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

    [[nodiscard]] virtual u8* GetOpenGLPointer() const = 0;

    [[nodiscard]] virtual u8* Map(std::size_t size) const = 0;

    virtual void Unmap(std::size_t size) const = 0;

    virtual void QueueFence(bool own) = 0;

    virtual void WaitFence() = 0;

    virtual void Discard() = 0;

    [[nodiscard]] virtual bool IsAvailable() = 0;

    virtual void Bind(GLenum target) const = 0;
};

class PersistentStagingBuffer final : public StagingBuffer {
public:
    explicit PersistentStagingBuffer(std::size_t size, bool is_readable);
    ~PersistentStagingBuffer() override;

    u8* GetOpenGLPointer() const override;

    u8* Map(std::size_t size) const override;

    void Unmap(std::size_t size) const override;

    void QueueFence(bool own) override;

    void WaitFence() override;

    void Discard() override;

    bool IsAvailable() override;

    void Bind(GLenum target) const override;

private:
    OGLBuffer buffer;
    GLsync sync{};
    u8* pointer{};
    bool is_readable{};
    bool owned{};
};

class CpuStagingBuffer final : public StagingBuffer {
public:
    explicit CpuStagingBuffer(std::size_t size, bool is_readable);
    ~CpuStagingBuffer() override;

    u8* GetOpenGLPointer() const override;

    u8* Map(std::size_t size) const override;

    void Unmap(std::size_t size) const override;

    void QueueFence(bool own) override;

    void WaitFence() override;

    void Discard() override;

    bool IsAvailable() override;

    void Bind(GLenum target) const override;

private:
    std::unique_ptr<u8[]> pointer;
};

} // namespace OpenGL
