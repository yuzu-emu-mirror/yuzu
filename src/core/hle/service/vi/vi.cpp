// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>

#include "common/alignment.h"
#include "common/scope_exit.h"
#include "core/core_timing.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nvdrv/devices/nvdisp_disp0.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/vi/vi.h"
#include "core/hle/service/vi/vi_m.h"
#include "video_core/renderer_base.h"
#include "video_core/video_core.h"

namespace Service {
namespace VI {

constexpr size_t SCREEN_REFRESH_RATE = 60;
constexpr u64 frame_ticks = static_cast<u64>(BASE_CLOCK_RATE / SCREEN_REFRESH_RATE);

class Parcel {
public:
    // This default size was chosen arbitrarily.
    static constexpr size_t DefaultBufferSize = 0x40;
    Parcel() : buffer(DefaultBufferSize) {}
    explicit Parcel(std::vector<u8> data) : buffer(std::move(data)) {}
    virtual ~Parcel() = default;

    template <typename T>
    T Read() {
        T val;
        std::memcpy(&val, buffer.data() + read_index, sizeof(T));
        read_index += sizeof(T);
        read_index = Common::AlignUp(read_index, 4);
        return val;
    }

    template <typename T>
    T ReadUnaligned() {
        T val;
        std::memcpy(&val, buffer.data() + read_index, sizeof(T));
        read_index += sizeof(T);
        return val;
    }

    std::vector<u8> ReadBlock(size_t length) {
        const u8* const begin = buffer.data() + read_index;
        const u8* const end = begin + length;
        std::vector<u8> data(begin, end);
        read_index += length;
        read_index = Common::AlignUp(read_index, 4);
        return data;
    }

    std::u16string ReadInterfaceToken() {
        u32 unknown = Read<u32_le>();
        u32 length = Read<u32_le>();

        std::u16string token{};

        for (u32 ch = 0; ch < length + 1; ++ch) {
            token.push_back(ReadUnaligned<u16_le>());
        }

        read_index = Common::AlignUp(read_index, 4);

        return token;
    }

    template <typename T>
    void Write(const T& val) {
        if (buffer.size() < write_index + sizeof(T))
            buffer.resize(buffer.size() + sizeof(T) + DefaultBufferSize);
        std::memcpy(buffer.data() + write_index, &val, sizeof(T));
        write_index += sizeof(T);
        write_index = Common::AlignUp(write_index, 4);
    }

    void Deserialize() {
        Header header{};
        std::memcpy(&header, buffer.data(), sizeof(Header));

        read_index = header.data_offset;
        DeserializeData();
    }

    std::vector<u8> Serialize() {
        ASSERT(read_index == 0);
        write_index = sizeof(Header);

        SerializeData();

        Header header{};
        header.data_offset = sizeof(Header);
        header.data_size = write_index - sizeof(Header);
        std::memcpy(buffer.data(), &header, sizeof(Header));

        return buffer;
    }

protected:
    virtual void SerializeData() {}

    virtual void DeserializeData() {}

private:
    struct Header {
        u32_le data_size;
        u32_le data_offset;
        u32_le objects_size;
        u32_le objects_offset;
    };
    static_assert(sizeof(Header) == 16, "ParcelHeader has wrong size");

    std::vector<u8> buffer;
    size_t read_index = 0;
    size_t write_index = 0;
};

class NativeWindow : public Parcel {
public:
    explicit NativeWindow(u32 id) : Parcel() {
        data.id = id;
    }
    ~NativeWindow() override = default;

protected:
    void SerializeData() override {
        Write(data);
    }

private:
    struct Data {
        u32_le magic = 2;
        u32_le process_id;
        u32_le id;
        INSERT_PADDING_BYTES(0xC);
        std::array<u8, 8> dispdrv = {'d', 'i', 's', 'p', 'd', 'r', 'v', '\0'};
        INSERT_PADDING_BYTES(8);
    };
    static_assert(sizeof(Data) == 0x28, "ParcelData has wrong size");

    Data data{};
};

class IGBPConnectRequestParcel : public Parcel {
public:
    explicit IGBPConnectRequestParcel(const std::vector<u8>& buffer) : Parcel(buffer) {
        Deserialize();
    }
    ~IGBPConnectRequestParcel() override = default;

    void DeserializeData() override {
        std::u16string token = ReadInterfaceToken();
        data = Read<Data>();
    }

    struct Data {
        u32_le unk;
        u32_le api;
        u32_le producer_controlled_by_app;
    };

    Data data;
};

class IGBPConnectResponseParcel : public Parcel {
public:
    explicit IGBPConnectResponseParcel(u32 width, u32 height) : Parcel() {
        data.width = width;
        data.height = height;
    }
    ~IGBPConnectResponseParcel() override = default;

protected:
    void SerializeData() override {
        Write(data);
    }

private:
    struct Data {
        u32_le width;
        u32_le height;
        u32_le transform_hint;
        u32_le num_pending_buffers;
        u32_le status;
    };
    static_assert(sizeof(Data) == 20, "ParcelData has wrong size");

    Data data{};
};

class IGBPSetPreallocatedBufferRequestParcel : public Parcel {
public:
    explicit IGBPSetPreallocatedBufferRequestParcel(const std::vector<u8>& buffer)
        : Parcel(buffer) {
        Deserialize();
    }
    ~IGBPSetPreallocatedBufferRequestParcel() override = default;

    void DeserializeData() override {
        std::u16string token = ReadInterfaceToken();
        data = Read<Data>();
        ASSERT(data.graphic_buffer_length == sizeof(IGBPBuffer));
        buffer = Read<IGBPBuffer>();
    }

    struct Data {
        u32_le slot;
        INSERT_PADDING_WORDS(1);
        u32_le graphic_buffer_length;
        INSERT_PADDING_WORDS(1);
    };

    Data data;
    IGBPBuffer buffer;
};

class IGBPSetPreallocatedBufferResponseParcel : public Parcel {
public:
    IGBPSetPreallocatedBufferResponseParcel() : Parcel() {}
    ~IGBPSetPreallocatedBufferResponseParcel() override = default;

protected:
    void SerializeData() override {
        // TODO(Subv): Find out what this means
        Write<u32>(0);
    }
};

class IGBPDequeueBufferRequestParcel : public Parcel {
public:
    explicit IGBPDequeueBufferRequestParcel(const std::vector<u8>& buffer) : Parcel(buffer) {
        Deserialize();
    }
    ~IGBPDequeueBufferRequestParcel() override = default;

    void DeserializeData() override {
        std::u16string token = ReadInterfaceToken();
        data = Read<Data>();
    }

    struct Data {
        u32_le pixel_format;
        u32_le width;
        u32_le height;
        u32_le get_frame_timestamps;
        u32_le usage;
    };

    Data data;
};

class IGBPDequeueBufferResponseParcel : public Parcel {
public:
    explicit IGBPDequeueBufferResponseParcel(u32 slot) : Parcel(), slot(slot) {}
    ~IGBPDequeueBufferResponseParcel() override = default;

protected:
    void SerializeData() override {
        Write(slot);
        // TODO(Subv): Find out how this Fence is used.
        std::array<u32_le, 11> fence = {};
        Write(fence);
        Write<u32_le>(0);
    }

    u32_le slot;
};

class IGBPRequestBufferRequestParcel : public Parcel {
public:
    explicit IGBPRequestBufferRequestParcel(const std::vector<u8>& buffer) : Parcel(buffer) {
        Deserialize();
    }
    ~IGBPRequestBufferRequestParcel() override = default;

    void DeserializeData() override {
        std::u16string token = ReadInterfaceToken();
        slot = Read<u32_le>();
    }

    u32_le slot;
};

class IGBPRequestBufferResponseParcel : public Parcel {
public:
    explicit IGBPRequestBufferResponseParcel(IGBPBuffer buffer) : Parcel(), buffer(buffer) {}
    ~IGBPRequestBufferResponseParcel() override = default;

protected:
    void SerializeData() override {
        // TODO(Subv): Find out what this all means
        Write<u32_le>(1);

        Write<u32_le>(sizeof(IGBPBuffer));
        Write<u32_le>(0); // Unknown

        Write(buffer);

        Write<u32_le>(0);
    }

    IGBPBuffer buffer;
};

class IGBPQueueBufferRequestParcel : public Parcel {
public:
    explicit IGBPQueueBufferRequestParcel(const std::vector<u8>& buffer) : Parcel(buffer) {
        Deserialize();
    }
    ~IGBPQueueBufferRequestParcel() override = default;

    void DeserializeData() override {
        std::u16string token = ReadInterfaceToken();
        data = Read<Data>();
    }

    struct Data {
        u32_le slot;
        INSERT_PADDING_WORDS(2);
        u32_le timestamp;
        INSERT_PADDING_WORDS(20);
    };
    static_assert(sizeof(Data) == 96, "ParcelData has wrong size");

    Data data;
};

class IGBPQueueBufferResponseParcel : public Parcel {
public:
    explicit IGBPQueueBufferResponseParcel(u32 width, u32 height) : Parcel() {
        data.width = width;
        data.height = height;
    }
    ~IGBPQueueBufferResponseParcel() override = default;

protected:
    void SerializeData() override {
        Write(data);
    }

private:
    struct Data {
        u32_le width;
        u32_le height;
        u32_le transform_hint;
        u32_le num_pending_buffers;
        u32_le status;
    };
    static_assert(sizeof(Data) == 20, "ParcelData has wrong size");

    Data data{};
};

class IHOSBinderDriver final : public ServiceFramework<IHOSBinderDriver> {
public:
    explicit IHOSBinderDriver(std::shared_ptr<NVFlinger> nv_flinger)
        : ServiceFramework("IHOSBinderDriver"), nv_flinger(std::move(nv_flinger)) {
        static const FunctionInfo functions[] = {
            {0, &IHOSBinderDriver::TransactParcel, "TransactParcel"},
            {1, &IHOSBinderDriver::AdjustRefcount, "AdjustRefcount"},
            {2, &IHOSBinderDriver::GetNativeHandle, "GetNativeHandle"},
            {3, nullptr, "TransactParcelAuto"},
        };
        RegisterHandlers(functions);
    }
    ~IHOSBinderDriver() = default;

private:
    enum class TransactionId {
        RequestBuffer = 1,
        SetBufferCount = 2,
        DequeueBuffer = 3,
        DetachBuffer = 4,
        DetachNextBuffer = 5,
        AttachBuffer = 6,
        QueueBuffer = 7,
        CancelBuffer = 8,
        Query = 9,
        Connect = 10,
        Disconnect = 11,

        AllocateBuffers = 13,
        SetPreallocatedBuffer = 14
    };

    void TransactParcel(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        u32 id = rp.Pop<u32>();
        auto transaction = static_cast<TransactionId>(rp.Pop<u32>());
        u32 flags = rp.Pop<u32>();

        auto& input_buffer = ctx.BufferDescriptorA()[0];
        std::vector<u8> input_data(input_buffer.Size());
        Memory::ReadBlock(input_buffer.Address(), input_data.data(), input_buffer.Size());

        auto& output_buffer = ctx.BufferDescriptorB()[0];

        auto buffer_queue = nv_flinger->GetBufferQueue(id);

        if (transaction == TransactionId::Connect) {
            IGBPConnectRequestParcel request{input_data};
            IGBPConnectResponseParcel response{1280, 720};
            auto response_buffer = response.Serialize();
            Memory::WriteBlock(output_buffer.Address(), response_buffer.data(),
                               output_buffer.Size());
        } else if (transaction == TransactionId::SetPreallocatedBuffer) {
            IGBPSetPreallocatedBufferRequestParcel request{input_data};

            buffer_queue->SetPreallocatedBuffer(request.data.slot, request.buffer);

            IGBPSetPreallocatedBufferResponseParcel response{};
            auto response_buffer = response.Serialize();
            Memory::WriteBlock(output_buffer.Address(), response_buffer.data(),
                               output_buffer.Size());
        } else if (transaction == TransactionId::DequeueBuffer) {
            IGBPDequeueBufferRequestParcel request{input_data};

            u32 slot = buffer_queue->DequeueBuffer(request.data.pixel_format, request.data.width,
                                                   request.data.height);

            IGBPDequeueBufferResponseParcel response{slot};
            auto response_buffer = response.Serialize();
            Memory::WriteBlock(output_buffer.Address(), response_buffer.data(),
                               output_buffer.Size());
        } else if (transaction == TransactionId::RequestBuffer) {
            IGBPRequestBufferRequestParcel request{input_data};

            auto& buffer = buffer_queue->RequestBuffer(request.slot);

            IGBPRequestBufferResponseParcel response{buffer};
            auto response_buffer = response.Serialize();
            Memory::WriteBlock(output_buffer.Address(), response_buffer.data(),
                               output_buffer.Size());
        } else if (transaction == TransactionId::QueueBuffer) {
            IGBPQueueBufferRequestParcel request{input_data};

            buffer_queue->QueueBuffer(request.data.slot);

            IGBPQueueBufferResponseParcel response{1280, 720};
            auto response_buffer = response.Serialize();
            Memory::WriteBlock(output_buffer.Address(), response_buffer.data(),
                               output_buffer.Size());
        } else {
            ASSERT_MSG(false, "Unimplemented");
        }

        LOG_WARNING(Service, "(STUBBED) called");
        IPC::RequestBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void AdjustRefcount(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        u32 id = rp.Pop<u32>();
        s32 addval = rp.PopRaw<s32>();
        u32 type = rp.Pop<u32>();

        LOG_WARNING(Service, "(STUBBED) called id=%u, addval=%08X, type=%08X", id, addval, type);
        IPC::RequestBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void GetNativeHandle(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        u32 id = rp.Pop<u32>();
        u32 unknown = rp.Pop<u32>();

        auto buffer_queue = nv_flinger->GetBufferQueue(id);

        // TODO(Subv): Find out what this actually is.

        LOG_WARNING(Service, "(STUBBED) called id=%u, unknown=%08X", id, unknown);
        IPC::RequestBuilder rb{ctx, 2, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(buffer_queue->GetNativeHandle());
    }

    std::shared_ptr<NVFlinger> nv_flinger;
};

class ISystemDisplayService final : public ServiceFramework<ISystemDisplayService> {
public:
    ISystemDisplayService() : ServiceFramework("ISystemDisplayService") {
        static const FunctionInfo functions[] = {
            {1200, nullptr, "GetZOrderCountMin"},
            {2205, &ISystemDisplayService::SetLayerZ, "SetLayerZ"},
        };
        RegisterHandlers(functions);
    }
    ~ISystemDisplayService() = default;

private:
    void SetLayerZ(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service, "(STUBBED) called");
        IPC::RequestParser rp{ctx};
        u64 layer_id = rp.Pop<u64>();
        u64 z_value = rp.Pop<u64>();

        IPC::RequestBuilder rb = rp.MakeBuilder(2, 0, 0, 0);
        rb.Push(RESULT_SUCCESS);
    }
};

class IManagerDisplayService final : public ServiceFramework<IManagerDisplayService> {
public:
    explicit IManagerDisplayService(std::shared_ptr<NVFlinger> nv_flinger)
        : ServiceFramework("IManagerDisplayService"), nv_flinger(std::move(nv_flinger)) {
        static const FunctionInfo functions[] = {
            {1020, &IManagerDisplayService::CloseDisplay, "CloseDisplay"},
            {1102, nullptr, "GetDisplayResolution"},
            {2010, &IManagerDisplayService::CreateManagedLayer, "CreateManagedLayer"},
            {6000, &IManagerDisplayService::AddToLayerStack, "AddToLayerStack"},
        };
        RegisterHandlers(functions);
    }
    ~IManagerDisplayService() = default;

private:
    void CloseDisplay(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service, "(STUBBED) called");
        IPC::RequestParser rp{ctx};
        u64 display = rp.Pop<u64>();

        IPC::RequestBuilder rb = rp.MakeBuilder(2, 0, 0, 0);
        rb.Push(RESULT_SUCCESS);
    }

    void CreateManagedLayer(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service, "(STUBBED) called");
        IPC::RequestParser rp{ctx};
        u32 unknown = rp.Pop<u32>();
        rp.Skip(1, false);
        u64 display = rp.Pop<u64>();
        u64 aruid = rp.Pop<u64>();

        u64 layer_id = nv_flinger->CreateLayer(display);

        IPC::RequestBuilder rb = rp.MakeBuilder(4, 0, 0, 0);
        rb.Push(RESULT_SUCCESS);
        rb.Push(layer_id);
    }

    void AddToLayerStack(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service, "(STUBBED) called");
        IPC::RequestParser rp{ctx};
        u32 stack = rp.Pop<u32>();
        u64 layer_id = rp.Pop<u64>();

        IPC::RequestBuilder rb = rp.MakeBuilder(2, 0, 0, 0);
        rb.Push(RESULT_SUCCESS);
    }

    std::shared_ptr<NVFlinger> nv_flinger;
};

void IApplicationDisplayService::GetRelayService(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IHOSBinderDriver>(nv_flinger);
}

void IApplicationDisplayService::GetSystemDisplayService(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<ISystemDisplayService>();
}

void IApplicationDisplayService::GetManagerDisplayService(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IManagerDisplayService>(nv_flinger);
}

void IApplicationDisplayService::GetIndirectDisplayTransactionService(
    Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IHOSBinderDriver>(nv_flinger);
}

void IApplicationDisplayService::OpenDisplay(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::RequestParser rp{ctx};
    auto name_buf = rp.PopRaw<std::array<u8, 0x40>>();
    auto end = std::find(name_buf.begin(), name_buf.end(), '\0');

    std::string name(name_buf.begin(), end);

    ASSERT_MSG(name == "Default", "Non-default displays aren't supported yet");

    IPC::RequestBuilder rb = rp.MakeBuilder(4, 0, 0, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push<u64>(nv_flinger->OpenDisplay(name));
}

void IApplicationDisplayService::CloseDisplay(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::RequestParser rp{ctx};
    u64 display_id = rp.Pop<u64>();

    IPC::RequestBuilder rb = rp.MakeBuilder(4, 0, 0, 0);
    rb.Push(RESULT_SUCCESS);
}

void IApplicationDisplayService::OpenLayer(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::RequestParser rp{ctx};
    auto name_buf = rp.PopRaw<std::array<u8, 0x40>>();
    auto end = std::find(name_buf.begin(), name_buf.end(), '\0');

    std::string display_name(name_buf.begin(), end);

    u64 layer_id = rp.Pop<u64>();
    u64 aruid = rp.Pop<u64>();

    auto& buffer = ctx.BufferDescriptorB()[0];

    u64 display_id = nv_flinger->OpenDisplay(display_name);
    u32 buffer_queue_id = nv_flinger->GetBufferQueueId(display_id, layer_id);

    NativeWindow native_window{buffer_queue_id};
    auto data = native_window.Serialize();
    Memory::WriteBlock(buffer.Address(), data.data(), data.size());

    IPC::RequestBuilder rb = rp.MakeBuilder(4, 0, 0, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push<u64>(data.size());
}

void IApplicationDisplayService::CreateStrayLayer(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::RequestParser rp{ctx};
    u32 flags = rp.Pop<u32>();
    u64 display_id = rp.Pop<u64>();

    auto& buffer = ctx.BufferDescriptorB()[0];

    // TODO(Subv): What's the difference between a Stray and a Managed layer?

    u64 layer_id = nv_flinger->CreateLayer(display_id);
    u32 buffer_queue_id = nv_flinger->GetBufferQueueId(display_id, layer_id);

    NativeWindow native_window{buffer_queue_id};
    auto data = native_window.Serialize();
    Memory::WriteBlock(buffer.Address(), data.data(), data.size());

    IPC::RequestBuilder rb = rp.MakeBuilder(6, 0, 0, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push(layer_id);
    rb.Push<u64>(data.size());
}

void IApplicationDisplayService::DestroyStrayLayer(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::RequestParser rp{ctx};
    u64 layer_id = rp.Pop<u64>();

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0, 0, 0);
    rb.Push(RESULT_SUCCESS);
}

void IApplicationDisplayService::SetLayerScalingMode(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::RequestParser rp{ctx};
    u32 scaling_mode = rp.Pop<u32>();
    u64 unknown = rp.Pop<u64>();

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0, 0, 0);
    rb.Push(RESULT_SUCCESS);
}

void IApplicationDisplayService::GetDisplayVsyncEvent(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::RequestParser rp{ctx};
    u64 display_id = rp.Pop<u64>();

    auto vsync_event = nv_flinger->GetVsyncEvent(display_id);

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 1, 0, 0);
    rb.Push(RESULT_SUCCESS);
    rb.PushCopyObjects(vsync_event);
}

IApplicationDisplayService::IApplicationDisplayService(std::shared_ptr<NVFlinger> nv_flinger)
    : ServiceFramework("IApplicationDisplayService"), nv_flinger(std::move(nv_flinger)) {
    static const FunctionInfo functions[] = {
        {100, &IApplicationDisplayService::GetRelayService, "GetRelayService"},
        {101, &IApplicationDisplayService::GetSystemDisplayService, "GetSystemDisplayService"},
        {102, &IApplicationDisplayService::GetManagerDisplayService, "GetManagerDisplayService"},
        {103, &IApplicationDisplayService::GetIndirectDisplayTransactionService,
         "GetIndirectDisplayTransactionService"},
        {1000, nullptr, "ListDisplays"},
        {1010, &IApplicationDisplayService::OpenDisplay, "OpenDisplay"},
        {1020, &IApplicationDisplayService::CloseDisplay, "CloseDisplay"},
        {2101, &IApplicationDisplayService::SetLayerScalingMode, "SetLayerScalingMode"},
        {2020, &IApplicationDisplayService::OpenLayer, "OpenLayer"},
        {2030, &IApplicationDisplayService::CreateStrayLayer, "CreateStrayLayer"},
        {2031, &IApplicationDisplayService::DestroyStrayLayer, "DestroyStrayLayer"},
        {5202, &IApplicationDisplayService::GetDisplayVsyncEvent, "GetDisplayVsyncEvent"},
    };
    RegisterHandlers(functions);
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<VI_M>()->InstallAsService(service_manager);
}

NVFlinger::NVFlinger() {
    // Add the different displays to the list of displays.
    Display default_{0, "Default"};
    Display external{1, "External"};
    Display edid{2, "Edid"};
    Display internal{3, "Internal"};

    displays.emplace_back(default_);
    displays.emplace_back(external);
    displays.emplace_back(edid);
    displays.emplace_back(internal);

    // Schedule the screen composition events
    composition_event =
        CoreTiming::RegisterEvent("ScreenCompositioin", [this](u64 userdata, int cycles_late) {
            Compose();
            CoreTiming::ScheduleEvent(frame_ticks - cycles_late, composition_event);
        });

    CoreTiming::ScheduleEvent(frame_ticks, composition_event);
}

NVFlinger::~NVFlinger() {
    CoreTiming::UnscheduleEvent(composition_event, 0);
}

u64 NVFlinger::OpenDisplay(const std::string& name) {
    LOG_WARNING(Service, "Opening display %s", name.c_str());

    // TODO(Subv): Currently we only support the Default display.
    ASSERT(name == "Default");

    auto itr = std::find_if(displays.begin(), displays.end(),
                            [&](const Display& display) { return display.name == name; });

    ASSERT(itr != displays.end());

    return itr->id;
}

u64 NVFlinger::CreateLayer(u64 display_id) {
    auto& display = GetDisplay(display_id);

    ASSERT_MSG(display.layers.empty(), "Only one layer is supported per display at the moment");

    u64 layer_id = next_layer_id++;
    u32 buffer_queue_id = next_buffer_queue_id++;
    auto buffer_queue = std::make_shared<BufferQueue>(buffer_queue_id, layer_id);
    display.layers.emplace_back(layer_id, buffer_queue);
    buffer_queues.emplace_back(std::move(buffer_queue));
    return layer_id;
}

u32 NVFlinger::GetBufferQueueId(u64 display_id, u64 layer_id) {
    const auto& layer = GetLayer(display_id, layer_id);
    return layer.buffer_queue->GetId();
}

Kernel::SharedPtr<Kernel::Event> NVFlinger::GetVsyncEvent(u64 display_id) {
    const auto& display = GetDisplay(display_id);
    return display.vsync_event;
}

std::shared_ptr<BufferQueue> NVFlinger::GetBufferQueue(u32 id) const {
    auto itr = std::find_if(buffer_queues.begin(), buffer_queues.end(),
                            [&](const auto& queue) { return queue->GetId() == id; });

    ASSERT(itr != buffer_queues.end());
    return *itr;
}

Display& NVFlinger::GetDisplay(u64 display_id) {
    auto itr = std::find_if(displays.begin(), displays.end(),
                            [&](const Display& display) { return display.id == display_id; });

    ASSERT(itr != displays.end());
    return *itr;
}

Layer& NVFlinger::GetLayer(u64 display_id, u64 layer_id) {
    auto& display = GetDisplay(display_id);

    auto itr = std::find_if(display.layers.begin(), display.layers.end(),
                            [&](const Layer& layer) { return layer.id == layer_id; });

    ASSERT(itr != display.layers.end());
    return *itr;
}

void NVFlinger::Compose() {
    for (auto& display : displays) {
        // Trigger vsync for this display at the end of drawing
        SCOPE_EXIT({ display.vsync_event->Signal(); });

        // Don't do anything for displays without layers.
        if (display.layers.empty())
            continue;

        // TODO(Subv): Support more than 1 layer.
        ASSERT_MSG(display.layers.size() == 1, "Max 1 layer per display is supported");

        Layer& layer = display.layers[0];
        auto& buffer_queue = layer.buffer_queue;

        // Search for a queued buffer and acquire it
        auto buffer = buffer_queue->AcquireBuffer();

        if (buffer == boost::none) {
            // There was no queued buffer to draw, render previous frame
            VideoCore::g_renderer->SwapBuffers({});
            continue;
        }

        auto& igbp_buffer = buffer->igbp_buffer;

        // Now send the buffer to the GPU for drawing.
        auto nvdrv = Nvidia::nvdrv.lock();
        ASSERT(nvdrv);

        // TODO(Subv): Support more than just disp0. The display device selection is probably based
        // on which display we're drawing (Default, Internal, External, etc)
        auto nvdisp = nvdrv->GetDevice<Nvidia::Devices::nvdisp_disp0>("/dev/nvdisp_disp0");
        ASSERT(nvdisp);

        nvdisp->flip(igbp_buffer.gpu_buffer_id, igbp_buffer.offset, igbp_buffer.format,
                     igbp_buffer.width, igbp_buffer.height, igbp_buffer.stride);

        buffer_queue->ReleaseBuffer(buffer->slot);
    }
}

BufferQueue::BufferQueue(u32 id, u64 layer_id) : id(id), layer_id(layer_id) {
    native_handle = Kernel::Event::Create(Kernel::ResetType::OneShot, "BufferQueue NativeHandle");
}

void BufferQueue::SetPreallocatedBuffer(u32 slot, IGBPBuffer& igbp_buffer) {
    Buffer buffer{};
    buffer.slot = slot;
    buffer.igbp_buffer = igbp_buffer;
    buffer.status = Buffer::Status::Free;

    LOG_WARNING(Service, "Adding graphics buffer %u", slot);

    queue.emplace_back(buffer);
}

u32 BufferQueue::DequeueBuffer(u32 pixel_format, u32 width, u32 height) {
    auto itr = std::find_if(queue.begin(), queue.end(), [&](const Buffer& buffer) {
        // Only consider free buffers. Buffers become free once again after they've been Acquired
        // and Released by the compositor, see the NVFlinger::Compose method.
        if (buffer.status != Buffer::Status::Free)
            return false;

        // Make sure that the parameters match.
        auto& igbp_buffer = buffer.igbp_buffer;
        return igbp_buffer.format == pixel_format && igbp_buffer.width == width &&
               igbp_buffer.height == height;
    });
    ASSERT(itr != queue.end());

    itr->status = Buffer::Status::Dequeued;
    return itr->slot;
}

const IGBPBuffer& BufferQueue::RequestBuffer(u32 slot) const {
    auto itr = std::find_if(queue.begin(), queue.end(),
                            [&](const Buffer& buffer) { return buffer.slot == slot; });
    ASSERT(itr != queue.end());
    ASSERT(itr->status == Buffer::Status::Dequeued);
    return itr->igbp_buffer;
}

void BufferQueue::QueueBuffer(u32 slot) {
    auto itr = std::find_if(queue.begin(), queue.end(),
                            [&](const Buffer& buffer) { return buffer.slot == slot; });
    ASSERT(itr != queue.end());
    ASSERT(itr->status == Buffer::Status::Dequeued);
    itr->status = Buffer::Status::Queued;
}

boost::optional<const BufferQueue::Buffer&> BufferQueue::AcquireBuffer() {
    auto itr = std::find_if(queue.begin(), queue.end(), [](const Buffer& buffer) {
        return buffer.status == Buffer::Status::Queued;
    });
    if (itr == queue.end())
        return boost::none;
    itr->status = Buffer::Status::Acquired;
    return *itr;
}

void BufferQueue::ReleaseBuffer(u32 slot) {
    auto itr = std::find_if(queue.begin(), queue.end(),
                            [&](const Buffer& buffer) { return buffer.slot == slot; });
    ASSERT(itr != queue.end());
    ASSERT(itr->status == Buffer::Status::Acquired);
    itr->status = Buffer::Status::Free;
}

Layer::Layer(u64 id, std::shared_ptr<BufferQueue> queue) : id(id), buffer_queue(std::move(queue)) {}

Display::Display(u64 id, std::string name) : id(id), name(std::move(name)) {
    vsync_event = Kernel::Event::Create(Kernel::ResetType::Pulse, "Display VSync Event");
}

} // namespace VI
} // namespace Service
