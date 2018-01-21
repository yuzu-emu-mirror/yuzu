// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <boost/optional.hpp>
#include "core/hle/kernel/event.h"
#include "core/hle/service/service.h"

namespace CoreTiming {
struct EventType;
}

namespace Service {
namespace VI {

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

class BufferQueue {
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

struct Layer {
    Layer(u64 id, std::shared_ptr<BufferQueue> queue);
    ~Layer() = default;

    u64 id;
    std::shared_ptr<BufferQueue> buffer_queue;
};

struct Display {
    Display(u64 id, std::string name);
    ~Display() = default;

    u64 id;
    std::string name;

    std::vector<Layer> layers;
    Kernel::SharedPtr<Kernel::Event> vsync_event;
};

class NVFlinger {
public:
    NVFlinger();
    ~NVFlinger();

    /// Opens the specified display and returns the id.
    u64 OpenDisplay(const std::string& name);

    /// Creates a layer on the specified display and returns the layer id.
    u64 CreateLayer(u64 display_id);

    /// Gets the buffer queue id of the specified layer in the specified display.
    u32 GetBufferQueueId(u64 display_id, u64 layer_id);

    /// Gets the vsync event for the specified display.
    Kernel::SharedPtr<Kernel::Event> GetVsyncEvent(u64 display_id);

    /// Obtains a buffer queue identified by the id.
    std::shared_ptr<BufferQueue> GetBufferQueue(u32 id) const;

    /// Performs a composition request to the emulated nvidia GPU and triggers the vsync events when
    /// finished.
    void Compose();

private:
    /// Returns the display identified by the specified id.
    Display& GetDisplay(u64 display_id);

    /// Returns the layer identified by the specified id in the desired display.
    Layer& GetLayer(u64 display_id, u64 layer_id);

    std::vector<Display> displays;
    std::vector<std::shared_ptr<BufferQueue>> buffer_queues;

    /// Id to use for the next layer that is created, this counter is shared among all displays.
    u64 next_layer_id = 1;
    /// Id to use for the next buffer queue that is created, this counter is shared among all
    /// layers.
    u32 next_buffer_queue_id = 1;

    /// CoreTiming event that handles screen composition.
    CoreTiming::EventType* composition_event;
};

class IApplicationDisplayService final : public ServiceFramework<IApplicationDisplayService> {
public:
    IApplicationDisplayService(std::shared_ptr<NVFlinger> nv_flinger);
    ~IApplicationDisplayService() = default;

private:
    void GetRelayService(Kernel::HLERequestContext& ctx);
    void GetSystemDisplayService(Kernel::HLERequestContext& ctx);
    void GetManagerDisplayService(Kernel::HLERequestContext& ctx);
    void GetIndirectDisplayTransactionService(Kernel::HLERequestContext& ctx);
    void OpenDisplay(Kernel::HLERequestContext& ctx);
    void CloseDisplay(Kernel::HLERequestContext& ctx);
    void SetLayerScalingMode(Kernel::HLERequestContext& ctx);
    void OpenLayer(Kernel::HLERequestContext& ctx);
    void CreateStrayLayer(Kernel::HLERequestContext& ctx);
    void DestroyStrayLayer(Kernel::HLERequestContext& ctx);
    void GetDisplayVsyncEvent(Kernel::HLERequestContext& ctx);

    std::shared_ptr<NVFlinger> nv_flinger;
};

/// Registers all VI services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace VI
} // namespace Service
