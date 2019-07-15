// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <glad/glad.h>
#include "common/assert.h"
#include "video_core/renderer_opengl/gl_device.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/renderer_opengl/gl_staging_buffer.h"

namespace OpenGL {

class PersistentStagingBuffer final : public StagingBuffer {
public:
    explicit PersistentStagingBuffer(std::size_t size, bool is_read_buffer)
        : is_read_buffer{is_read_buffer} {
        constexpr GLenum storage_read = GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT;
        constexpr GLenum storage_write = GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT;
        constexpr GLenum map_read = GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT;
        constexpr GLenum map_write = GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT |
                                     GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
        const GLenum storage = is_read_buffer ? storage_read : storage_write;
        const GLenum map = is_read_buffer ? map_read : map_write;

        buffer.Create();
        glNamedBufferStorage(buffer.handle, static_cast<GLsizeiptr>(size), nullptr, storage);
        pointer = reinterpret_cast<u8*>(
            glMapNamedBufferRange(buffer.handle, 0, static_cast<GLsizeiptr>(size), map));
    }

    ~PersistentStagingBuffer() override {
        if (sync) {
            glDeleteSync(sync);
        }
    }

    u8* GetOpenGLPointer() const override {
        // Operations with a bound OpenGL buffer start with an offset of 0.
        return nullptr;
    }

    u8* Map([[maybe_unused]] std::size_t size) const override {
        return pointer;
    }

    void Unmap(std::size_t size) const override {
        if (!is_read_buffer) {
            // We flush the buffer on write operations
            glFlushMappedNamedBufferRange(buffer.handle, 0, size);
        }
    }

    void QueueFence(bool own) override {
        DEBUG_ASSERT(!sync);
        owned = own;
        sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }

    void WaitFence() override {
        DEBUG_ASSERT(sync);
        switch (glClientWaitSync(sync, 0, GL_TIMEOUT_IGNORED)) {
        case GL_ALREADY_SIGNALED:
        case GL_CONDITION_SATISFIED:
            break;
        case GL_TIMEOUT_EXPIRED:
        case GL_WAIT_FAILED:
            UNREACHABLE_MSG("Fence wait failed");
            break;
        }
        Discard();
    }

    void Discard() override {
        DEBUG_ASSERT(sync);
        glDeleteSync(sync);
        sync = nullptr;
        owned = false;
    }

    bool IsAvailable() override {
        if (owned) {
            return false;
        }
        if (!sync) {
            return true;
        }
        switch (glClientWaitSync(sync, 0, 0)) {
        case GL_TIMEOUT_EXPIRED:
            // The fence is unavailable
            return false;
        case GL_ALREADY_SIGNALED:
        case GL_CONDITION_SATISFIED:
            break;
        case GL_WAIT_FAILED:
            UNREACHABLE_MSG("Fence wait failed");
            break;
        default:
            UNREACHABLE_MSG("Unknown glClientWaitSync result");
            break;
        }
        // The fence has been signaled, we can destroy it
        glDeleteSync(sync);
        sync = nullptr;
        return true;
    }

    void Bind(GLenum target) const override {
        glBindBuffer(target, buffer.handle);
    }

private:
    OGLBuffer buffer;
    GLsync sync{};
    u8* pointer{};
    bool is_read_buffer{};
    bool owned{};
};

class CpuStagingBuffer final : public StagingBuffer {
public:
    explicit CpuStagingBuffer(std::size_t size) : pointer{std::make_unique<u8[]>(size)} {}

    ~CpuStagingBuffer() override = default;

    u8* Map([[maybe_unused]] std::size_t size) const override {
        return pointer.get();
    }

    u8* GetOpenGLPointer() const override {
        return pointer.get();
    }

    void Unmap([[maybe_unused]] std::size_t size) const override {}

    void QueueFence(bool own) override {
        // We don't queue anything here
    }

    void WaitFence() override {
        // CPU operations are immediate, we don't wait for anything
    }

    void Discard() override {
        UNREACHABLE_MSG("CpuStagingBuffer doesn't support deferred operations");
    }

    bool IsAvailable() override {
        // A CPU buffer is always available, operations are immediate
        return true;
    }

    void Bind(GLenum target) const override {
        // OpenGL operations that use CPU buffers need that the target is zero
        glBindBuffer(target, 0);
    }

private:
    std::unique_ptr<u8[]> pointer;
};

StagingBufferCache::StagingBufferCache(const Device& device)
    : VideoCommon::StagingBufferCache<StagingBuffer>{!device.HasBrokenPBOStreaming()},
      device{device} {}

StagingBufferCache::~StagingBufferCache() = default;

std::unique_ptr<StagingBuffer> StagingBufferCache::CreateBuffer(std::size_t size, bool is_flush) {
    if (device.HasBrokenPBOStreaming()) {
        return std::unique_ptr<StagingBuffer>(new CpuStagingBuffer(size));
    } else {
        return std::unique_ptr<StagingBuffer>(new PersistentStagingBuffer(size, is_flush));
    }
}

} // namespace OpenGL
