// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include <boost/optional.hpp>
#include "common/swap.h"
#include "core/hle/kernel/event.h"

namespace CoreTiming {
struct EventType;
}

namespace Service {
namespace NVFlinger {

struct IGBPBuffer {
    u32_le magic;
    u32_le width;
    u32_le height;
    u32_le stride;
    u32_le format;
    u32_le usage;
    INSERT_PADDING_WORDS(1);
    u32_le index;
    INSERT_PADDING_WORDS(3);
    u32_le gpu_buffer_id;
    INSERT_PADDING_WORDS(17);
    u32_le nvmap_handle;
    u32_le offset;
    INSERT_PADDING_WORDS(60);
};

static_assert(sizeof(IGBPBuffer) == 0x16C, "IGBPBuffer has wrong size");

class BufferQueue final {
public:
    enum class QueryType {
        NativeWindowWidth = 0,
        NativeWindowHeight = 1,
        NativeWindowFormat = 2,
    };

    BufferQueue(u32 id, u64 layer_id);
    ~BufferQueue() = default;

    struct Buffer {
        enum class Status { Free = 0, Queued = 1, Dequeued = 2, Acquired = 3 };

        u32 slot;
        Status status = Status::Free;
        IGBPBuffer igbp_buffer;
    };

    void SetPreallocatedBuffer(u32 slot, IGBPBuffer& buffer);
    u32 DequeueBuffer(u32 pixel_format, u32 width, u32 height);
    const IGBPBuffer& RequestBuffer(u32 slot) const;
    void QueueBuffer(u32 slot);
    boost::optional<const Buffer&> AcquireBuffer();
    void ReleaseBuffer(u32 slot);
    u32 Query(QueryType type);

    u32 GetId() const {
        return id;
    }

    Kernel::SharedPtr<Kernel::Event> GetNativeHandle() const {
        return native_handle;
    }

private:
    u32 id;
    u64 layer_id;

    std::vector<Buffer> queue;
    Kernel::SharedPtr<Kernel::Event> native_handle;
};

} // namespace NVFlinger
} // namespace Service
