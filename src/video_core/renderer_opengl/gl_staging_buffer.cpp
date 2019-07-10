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

StagingBufferCache::StagingBufferCache(const Device& device)
    : VideoCommon::StagingBufferCache<StagingBuffer>{!device.HasBrokenPBOStreaming()},
      device{device} {}

StagingBufferCache::~StagingBufferCache() = default;

std::unique_ptr<StagingBuffer> StagingBufferCache::CreateBuffer(std::size_t size, bool is_flush) {
    if (device.HasBrokenPBOStreaming()) {
        return std::unique_ptr<StagingBuffer>(new CpuStagingBuffer(size, is_flush));
    } else {
        return std::unique_ptr<StagingBuffer>(new PersistentStagingBuffer(size, is_flush));
    }
}

PersistentStagingBuffer::PersistentStagingBuffer(std::size_t size, bool is_readable)
    : is_readable{is_readable} {
    buffer.Create();
    glNamedBufferStorage(buffer.handle, static_cast<GLsizeiptr>(size), nullptr,
                         GL_MAP_PERSISTENT_BIT |
                             (is_readable ? GL_MAP_READ_BIT : GL_MAP_WRITE_BIT));
    pointer = reinterpret_cast<u8*>(glMapNamedBufferRange(
        buffer.handle, 0, static_cast<GLsizeiptr>(size),
        GL_MAP_PERSISTENT_BIT | (is_readable ? GL_MAP_READ_BIT
                                             : (GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT |
                                                GL_MAP_UNSYNCHRONIZED_BIT))));
}

PersistentStagingBuffer::~PersistentStagingBuffer() {
    if (sync) {
        glDeleteSync(sync);
    }
}

u8* PersistentStagingBuffer::GetOpenGLPointer() const {
    return nullptr;
}

u8* PersistentStagingBuffer::Map([[maybe_unused]] std::size_t size) const {
    return pointer;
}

void PersistentStagingBuffer::Unmap(std::size_t size) const {
    if (!is_readable) {
        glFlushMappedNamedBufferRange(buffer.handle, 0, size);
    }
}

void PersistentStagingBuffer::QueueFence(bool own) {
    DEBUG_ASSERT(!sync);
    owned = own;
    sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void PersistentStagingBuffer::WaitFence() {
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

void PersistentStagingBuffer::Discard() {
    DEBUG_ASSERT(sync);
    glDeleteSync(sync);
    sync = nullptr;
    owned = false;
}

bool PersistentStagingBuffer::IsAvailable() {
    if (owned) {
        return false;
    }
    if (!sync) {
        return true;
    }
    switch (glClientWaitSync(sync, 0, 0)) {
    case GL_TIMEOUT_EXPIRED:
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
    glDeleteSync(sync);
    sync = nullptr;
    return true;
}

void PersistentStagingBuffer::Bind(GLenum target) const {
    glBindBuffer(target, buffer.handle);
}

CpuStagingBuffer::CpuStagingBuffer(std::size_t size, bool is_readable)
    : pointer{std::make_unique<u8[]>(size)} {}

CpuStagingBuffer::~CpuStagingBuffer() = default;

u8* CpuStagingBuffer::Map(std::size_t size) const {
    return pointer.get();
}

u8* CpuStagingBuffer::GetOpenGLPointer() const {
    return pointer.get();
}

void CpuStagingBuffer::Unmap(std::size_t size) const {}

void CpuStagingBuffer::QueueFence(bool own) {}

void CpuStagingBuffer::WaitFence() {}

void CpuStagingBuffer::Discard() {
    UNREACHABLE();
}

bool CpuStagingBuffer::IsAvailable() {
    return true;
}

void CpuStagingBuffer::Bind(GLenum target) const {
    glBindBuffer(target, 0);
}

} // namespace OpenGL
