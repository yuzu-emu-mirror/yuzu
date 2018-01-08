// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/alignment.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/vi/vi.h"
#include "core/hle/service/vi/vi_m.h"

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
    INSERT_PADDING_WORDS(61);
};

static_assert(sizeof(IGBPBuffer) == 0x16C, "IGBPBuffer has wrong size");

class Parcel {
public:
    // This default size was chosen arbitrarily.
    static constexpr size_t DefaultBufferSize = 0x40;
    Parcel() : buffer(DefaultBufferSize) {}
    Parcel(std::vector<u8> data) : buffer(std::move(data)) {}
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
        std::vector<u8> data(length);
        std::memcpy(data.data(), buffer.data() + read_index, length);
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
    virtual void SerializeData(){};

    virtual void DeserializeData(){};

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
    NativeWindow(u32 id) : Parcel() {
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
        std::array<u8, 8> dspdrv = {'d', 's', 'p', 'd', 'r', 'v'};
        INSERT_PADDING_BYTES(8);
    };
    static_assert(sizeof(Data) == 0x28, "ParcelData has wrong size");

    Data data{};
};

class IGBPConnectRequestParcel : public Parcel {
public:
    IGBPConnectRequestParcel(const std::vector<u8>& buffer) : Parcel(buffer) {
        Deserialize();
    }
    ~IGBPConnectRequestParcel() override = default;

    void DeserializeData() {
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
    IGBPConnectResponseParcel(u32 width, u32 height) : Parcel() {
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
    IGBPSetPreallocatedBufferRequestParcel(const std::vector<u8>& buffer) : Parcel(buffer) {
        Deserialize();
    }
    ~IGBPSetPreallocatedBufferRequestParcel() override = default;

    void DeserializeData() {
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
    IGBPDequeueBufferRequestParcel(const std::vector<u8>& buffer) : Parcel(buffer) {
        Deserialize();
    }
    ~IGBPDequeueBufferRequestParcel() override = default;

    void DeserializeData() {
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
    IGBPDequeueBufferResponseParcel(u32 slot) : Parcel(), slot(slot) {}
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
    IGBPRequestBufferRequestParcel(const std::vector<u8>& buffer) : Parcel(buffer) {
        Deserialize();
    }
    ~IGBPRequestBufferRequestParcel() override = default;

    void DeserializeData() {
        std::u16string token = ReadInterfaceToken();
        slot = Read<u32_le>();
    }

    u32_le slot;
};

class IGBPRequestBufferResponseParcel : public Parcel {
public:
    IGBPRequestBufferResponseParcel(IGBPBuffer buffer) : Parcel(), buffer(buffer) {}
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
    IGBPQueueBufferRequestParcel(const std::vector<u8>& buffer) : Parcel(buffer) {
        Deserialize();
    }
    ~IGBPQueueBufferRequestParcel() override = default;

    void DeserializeData() {
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
    IGBPQueueBufferResponseParcel(u32 width, u32 height) : Parcel() {
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
    IHOSBinderDriver() : ServiceFramework("IHOSBinderDriver") {
        static const FunctionInfo functions[] = {
            {0, &IHOSBinderDriver::TransactParcel, "TransactParcel"},
            {1, &IHOSBinderDriver::AdjustRefcount, "AdjustRefcount"},
            {2, nullptr, "GetNativeHandle"},
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

        if (transaction == TransactionId::Connect) {
            IGBPConnectRequestParcel request{input_data};
            IGBPConnectResponseParcel response{1280, 720};
            auto response_buffer = response.Serialize();
            Memory::WriteBlock(output_buffer.Address(), response_buffer.data(),
                               output_buffer.Size());
        } else if (transaction == TransactionId::SetPreallocatedBuffer) {
            IGBPSetPreallocatedBufferRequestParcel request{input_data};

            LOG_WARNING(Service, "Adding graphics buffer %u", request.data.slot);
            graphic_buffers.push_back(request.buffer);

            IGBPSetPreallocatedBufferResponseParcel response{};
            auto response_buffer = response.Serialize();
            Memory::WriteBlock(output_buffer.Address(), response_buffer.data(),
                               output_buffer.Size());
        } else if (transaction == TransactionId::DequeueBuffer) {
            IGBPDequeueBufferRequestParcel request{input_data};

            IGBPDequeueBufferResponseParcel response{0};
            auto response_buffer = response.Serialize();
            Memory::WriteBlock(output_buffer.Address(), response_buffer.data(),
                               output_buffer.Size());
        } else if (transaction == TransactionId::RequestBuffer) {
            IGBPRequestBufferRequestParcel request{input_data};

            auto& buffer = graphic_buffers[request.slot];
            IGBPRequestBufferResponseParcel response{buffer};
            auto response_buffer = response.Serialize();
            Memory::WriteBlock(output_buffer.Address(), response_buffer.data(),
                               output_buffer.Size());
        } else if (transaction == TransactionId::QueueBuffer) {
            IGBPQueueBufferRequestParcel request{input_data};

            IGBPQueueBufferResponseParcel response{1280, 720};
            auto response_buffer = response.Serialize();
            Memory::WriteBlock(output_buffer.Address(), response_buffer.data(),
                               output_buffer.Size());

            // TODO(Subv): Start drawing here?
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

    std::vector<IGBPBuffer> graphic_buffers;
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
    IManagerDisplayService() : ServiceFramework("IManagerDisplayService") {
        static const FunctionInfo functions[] = {
            {1102, nullptr, "GetDisplayResolution"},
            {2010, &IManagerDisplayService::CreateManagedLayer, "CreateManagedLayer"},
            {6000, &IManagerDisplayService::AddToLayerStack, "AddToLayerStack"},
        };
        RegisterHandlers(functions);
    }
    ~IManagerDisplayService() = default;

private:
    void CreateManagedLayer(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service, "(STUBBED) called");
        IPC::RequestParser rp{ctx};
        u32 unknown = rp.Pop<u32>();
        rp.Skip(1, false);
        u64 display = rp.Pop<u64>();
        u64 aruid = rp.Pop<u64>();

        IPC::RequestBuilder rb = rp.MakeBuilder(4, 0, 0, 0);
        rb.Push(RESULT_SUCCESS);
        rb.Push<u64>(1); // LayerId
    }

    void AddToLayerStack(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service, "(STUBBED) called");
        IPC::RequestParser rp{ctx};
        u32 stack = rp.Pop<u32>();
        u64 layer_id = rp.Pop<u64>();

        IPC::RequestBuilder rb = rp.MakeBuilder(2, 0, 0, 0);
        rb.Push(RESULT_SUCCESS);
    }
};

void IApplicationDisplayService::GetRelayService(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");

    IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IHOSBinderDriver>();
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
    rb.PushIpcInterface<IManagerDisplayService>();
}

void IApplicationDisplayService::OpenDisplay(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::RequestParser rp{ctx};
    auto data = rp.PopRaw<std::array<u8, 0x40>>();
    std::string display_name(data.begin(), data.end());

    IPC::RequestBuilder rb = rp.MakeBuilder(4, 0, 0, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push<u64>(9); // DisplayId
}

void IApplicationDisplayService::OpenLayer(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::RequestParser rp{ctx};
    auto name_buf = rp.PopRaw<std::array<u8, 0x40>>();
    u64 layer_id = rp.Pop<u64>();
    u64 aruid = rp.Pop<u64>();

    std::string display_name(name_buf.begin(), name_buf.end());

    auto& buffer = ctx.BufferDescriptorB()[0];

    NativeWindow native_window{1};
    auto data = native_window.Serialize();
    Memory::WriteBlock(buffer.Address(), data.data(), data.size());

    IPC::RequestBuilder rb = rp.MakeBuilder(4, 0, 0, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push<u64>(1280 * 720); // NativeWindowSize
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

    IPC::RequestBuilder rb = rp.MakeBuilder(2, 1, 0, 0);
    rb.Push(RESULT_SUCCESS);
    rb.PushCopyObjects(vsync_event);
}

IApplicationDisplayService::IApplicationDisplayService()
    : ServiceFramework("IApplicationDisplayService") {
    static const FunctionInfo functions[] = {
        {100, &IApplicationDisplayService::GetRelayService, "GetRelayService"},
        {101, &IApplicationDisplayService::GetSystemDisplayService, "GetSystemDisplayService"},
        {102, &IApplicationDisplayService::GetManagerDisplayService, "GetManagerDisplayService"},
        {103, nullptr, "GetIndirectDisplayTransactionService"},
        {1000, nullptr, "ListDisplays"},
        {1010, &IApplicationDisplayService::OpenDisplay, "OpenDisplay"},
        {2101, &IApplicationDisplayService::SetLayerScalingMode, "SetLayerScalingMode"},
        {2020, &IApplicationDisplayService::OpenLayer, "OpenLayer"},
        {5202, &IApplicationDisplayService::GetDisplayVsyncEvent, "GetDisplayVsyncEvent"},
    };
    RegisterHandlers(functions);

    vsync_event = Kernel::Event::Create(Kernel::ResetType::OneShot, "Display VSync Event");
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<VI_M>()->InstallAsService(service_manager);
}

} // namespace VI
} // namespace Service
