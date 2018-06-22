// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/alignment.h"
#include "common/logging/log.h"
#include "core/core_timing.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/audio/audren_u.h"

namespace Service::Audio {

/// TODO(bunnei): Find a proper value for the audio_ticks
constexpr u64 audio_ticks{static_cast<u64>(CoreTiming::BASE_CLOCK_RATE / 200)};

class IAudioRenderer final : public ServiceFramework<IAudioRenderer> {
public:
    IAudioRenderer() : ServiceFramework("IAudioRenderer") {
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetAudioRendererSampleRate"},
            {1, nullptr, "GetAudioRendererSampleCount"},
            {2, nullptr, "GetAudioRendererMixBufferCount"},
            {3, nullptr, "GetAudioRendererState"},
            {4, &IAudioRenderer::RequestUpdateAudioRenderer, "RequestUpdateAudioRenderer"},
            {5, &IAudioRenderer::StartAudioRenderer, "StartAudioRenderer"},
            {6, &IAudioRenderer::StopAudioRenderer, "StopAudioRenderer"},
            {7, &IAudioRenderer::QuerySystemEvent, "QuerySystemEvent"},
            {8, nullptr, "SetAudioRendererRenderingTimeLimit"},
            {9, nullptr, "GetAudioRendererRenderingTimeLimit"},
            {10, nullptr, "RequestUpdateAudioRendererAuto"},
            {11, nullptr, "ExecuteAudioRendererRendering"},
        };
        RegisterHandlers(functions);

        system_event =
            Kernel::Event::Create(Kernel::ResetType::OneShot, "IAudioRenderer:SystemEvent");

        // Register event callback to update the Audio Buffer
        audio_event = CoreTiming::RegisterEvent(
            "IAudioRenderer::UpdateAudioCallback", [this](u64 userdata, int cycles_late) {
                UpdateAudioCallback();
                CoreTiming::ScheduleEvent(audio_ticks - cycles_late, audio_event);
            });

        // Start the audio event
        CoreTiming::ScheduleEvent(audio_ticks, audio_event);
    }
    ~IAudioRenderer() {
        CoreTiming::UnscheduleEvent(audio_event, 0);
    }

private:
    void UpdateAudioCallback() {
        system_event->Signal();
    }

    void RequestUpdateAudioRenderer(Kernel::HLERequestContext& ctx) {
        AudioRendererConfig config;
        auto buf = ctx.ReadBuffer();
        std::memcpy(&config, buf.data(), sizeof(AudioRendererConfig));

        AudioRendererResponse response_data{config};

        ASSERT(ctx.GetWriteBufferSize() == response_data.total_size);

        std::vector<u8> output(response_data.total_size);
        std::memcpy(output.data(), &response_data, sizeof(AudioRendererResponse));
        std::vector<MemoryPoolEntry> memory_pool(config.memory_pools_size / 0x20);
        for (auto& entry : memory_pool) {
            entry.state = 5;
        }
        std::memcpy(output.data() + sizeof(AudioRendererResponse), memory_pool.data(),
                    response_data.memory_pools_size);

        ctx.WriteBuffer(output);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);

        NGLOG_WARNING(Service_Audio, "(STUBBED) called");
    }

    void StartAudioRenderer(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};

        rb.Push(RESULT_SUCCESS);

        NGLOG_WARNING(Service_Audio, "(STUBBED) called");
    }

    void StopAudioRenderer(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};

        rb.Push(RESULT_SUCCESS);

        NGLOG_WARNING(Service_Audio, "(STUBBED) called");
    }

    void QuerySystemEvent(Kernel::HLERequestContext& ctx) {
        // system_event->Signal();

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(system_event);

        NGLOG_WARNING(Service_Audio, "(STUBBED) called");
    }

    struct MemoryPoolEntry {
        u32_le state;
        u32_le unknown_4;
        u32_le unknown_8;
        u32_le unknown_c;
    };
    static_assert(sizeof(MemoryPoolEntry) == 0x10, "MemoryPoolEntry has wrong size");

    struct AudioRendererConfig {
        u32 revision;
        u32 behavior_size;
        u32 memory_pools_size;
        u32 voices_size;
        u32 voice_resource_size;
        u32 effects_size;
        u32 mixes_size;
        u32 sinks_size;
        u32 performance_buffer_size;
        INSERT_PADDING_WORDS(6);
        u32 total_size;
    };
    static_assert(sizeof(AudioRendererConfig) == 0x40, "AudioRendererConfig has wrong size");

    struct AudioRendererResponse {
        AudioRendererResponse(const AudioRendererConfig& config) {
            revision = config.revision;
            error_info_size = 0xb0;
            memory_pools_size = (config.memory_pools_size / 0x20) * 0x10;
            voices_size = (config.voices_size / 0x170) * 0x10;
            effects_size = (config.effects_size / 0xC0) * 0x10;
            sinks_size = (config.sinks_size / 0x140) * 0x20;
            performance_manager_size = 0x10;
            total_size = sizeof(AudioRendererResponse) + error_info_size + memory_pools_size +
                         voices_size + effects_size + sinks_size + performance_manager_size;
        }

        u32_le revision;
        u32_le error_info_size;
        u32_le memory_pools_size;
        u32_le voices_size;
        u32_le unknown_10;
        u32_le effects_size;
        u32_le unknown_18;
        u32_le sinks_size;
        u32_le performance_manager_size;
        INSERT_PADDING_WORDS(6);
        u32_le total_size;
    };
    static_assert(sizeof(AudioRendererResponse) == 0x40, "AudioRendererResponse has wrong size");

    /// This is used to trigger the audio event callback.
    CoreTiming::EventType* audio_event;

    Kernel::SharedPtr<Kernel::Event> system_event;
};

class IAudioDevice final : public ServiceFramework<IAudioDevice> {
public:
    IAudioDevice() : ServiceFramework("IAudioDevice") {
        static const FunctionInfo functions[] = {
            {0, &IAudioDevice::ListAudioDeviceName, "ListAudioDeviceName"},
            {1, &IAudioDevice::SetAudioDeviceOutputVolume, "SetAudioDeviceOutputVolume"},
            {2, nullptr, "GetAudioDeviceOutputVolume"},
            {3, &IAudioDevice::GetActiveAudioDeviceName, "GetActiveAudioDeviceName"},
            {4, &IAudioDevice::QueryAudioDeviceSystemEvent, "QueryAudioDeviceSystemEvent"},
            {5, &IAudioDevice::GetActiveChannelCount, "GetActiveChannelCount"},
            {6, &IAudioDevice::ListAudioDeviceName,
             "ListAudioDeviceNameAuto"}, // TODO(ogniK): Confirm if autos are identical to non auto
            {7, &IAudioDevice::SetAudioDeviceOutputVolume, "SetAudioDeviceOutputVolumeAuto"},
            {8, nullptr, "GetAudioDeviceOutputVolumeAuto"},
            {10, &IAudioDevice::GetActiveAudioDeviceName, "GetActiveAudioDeviceNameAuto"},
            {11, nullptr, "QueryAudioDeviceInputEvent"},
            {12, nullptr, "QueryAudioDeviceOutputEvent"},
        };
        RegisterHandlers(functions);

        buffer_event =
            Kernel::Event::Create(Kernel::ResetType::OneShot, "IAudioOutBufferReleasedEvent");
    }

private:
    void ListAudioDeviceName(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_Audio, "(STUBBED) called");
        IPC::RequestParser rp{ctx};

        const std::string audio_interface = "AudioInterface";
        ctx.WriteBuffer(audio_interface.c_str(), audio_interface.size());

        IPC::ResponseBuilder rb = rp.MakeBuilder(3, 0, 0);
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(1);
    }

    void SetAudioDeviceOutputVolume(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_Audio, "(STUBBED) called");

        IPC::RequestParser rp{ctx};
        f32 volume = static_cast<f32>(rp.Pop<u32>());

        auto file_buffer = ctx.ReadBuffer();
        auto end = std::find(file_buffer.begin(), file_buffer.end(), '\0');

        IPC::ResponseBuilder rb = rp.MakeBuilder(2, 0, 0);
        rb.Push(RESULT_SUCCESS);
    }

    void GetActiveAudioDeviceName(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_Audio, "(STUBBED) called");
        IPC::RequestParser rp{ctx};

        const std::string audio_interface = "AudioDevice";
        ctx.WriteBuffer(audio_interface.c_str(), audio_interface.size());

        IPC::ResponseBuilder rb = rp.MakeBuilder(3, 0, 0);
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(1);
    }

    void QueryAudioDeviceSystemEvent(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_Audio, "(STUBBED) called");

        buffer_event->Signal();

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(buffer_event);
    }

    void GetActiveChannelCount(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_Audio, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(1);
    }

    Kernel::SharedPtr<Kernel::Event> buffer_event;

}; // namespace Audio

AudRenU::AudRenU() : ServiceFramework("audren:u") {
    static const FunctionInfo functions[] = {
        {0, &AudRenU::OpenAudioRenderer, "OpenAudioRenderer"},
        {1, &AudRenU::GetAudioRendererWorkBufferSize, "GetAudioRendererWorkBufferSize"},
        {2, &AudRenU::GetAudioDevice, "GetAudioDevice"},
        {3, nullptr, "OpenAudioRendererAuto"},
        {4, nullptr, "GetAudioDeviceServiceWithRevisionInfo"},
    };
    RegisterHandlers(functions);
}

void AudRenU::OpenAudioRenderer(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};

    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<Audio::IAudioRenderer>();

    NGLOG_DEBUG(Service_Audio, "called");
}

void AudRenU::GetAudioRendererWorkBufferSize(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    auto params = rp.PopRaw<AudioRendererParameters>();

    u64 buffer_sz = Common::AlignUp(4 * params.unknown8, 0x40);
    buffer_sz += params.unknownC * 1024;
    buffer_sz += 0x940 * (params.unknownC + 1);
    buffer_sz += 0x3F0 * params.voice_count;
    buffer_sz += Common::AlignUp(8 * (params.unknownC + 1), 0x10);
    buffer_sz += Common::AlignUp(8 * params.voice_count, 0x10);
    buffer_sz +=
        Common::AlignUp((0x3C0 * (params.sink_count + params.unknownC) + 4 * params.sample_count) *
                            (params.unknown8 + 6),
                        0x40);

    if (IsFeatureSupported(AudioFeatures::Splitter, params.magic)) {
        u32 count = params.unknownC + 1;
        u64 node_count = Common::AlignUp(count, 0x40);
        u64 node_state_buffer_sz =
            4 * (node_count * node_count) + 0xC * node_count + 2 * (node_count / 8);
        u64 edge_matrix_buffer_sz = 0;
        node_count = Common::AlignUp(count * count, 0x40);
        if (node_count >> 31 != 0) {
            edge_matrix_buffer_sz = (node_count | 7) / 8;
        } else {
            edge_matrix_buffer_sz = node_count / 8;
        }
        buffer_sz += Common::AlignUp(node_state_buffer_sz + edge_matrix_buffer_sz, 0x10);
    }

    buffer_sz += 0x20 * (params.effect_count + 4 * params.voice_count) + 0x50;
    if (IsFeatureSupported(AudioFeatures::Splitter, params.magic)) {
        buffer_sz += 0xE0 * params.unknown2c;
        buffer_sz += 0x20 * params.splitter_count;
        buffer_sz += Common::AlignUp(4 * params.unknown2c, 0x10);
    }
    buffer_sz = Common::AlignUp(buffer_sz, 0x40) + 0x170 * params.sink_count;
    u64 output_sz = buffer_sz + 0x280 * params.sink_count + 0x4B0 * params.effect_count +
                    ((params.voice_count * 256) | 0x40);

    if (params.unknown1c >= 1) {
        output_sz = Common::AlignUp(((16 * params.sink_count + 16 * params.effect_count +
                                      16 * params.voice_count + 16) +
                                     0x658) *
                                            (params.unknown1c + 1) +
                                        0xc0,
                                    0x40) +
                    output_sz;
    }
    output_sz = Common::AlignUp(output_sz + 0x1807e, 0x1000);

    IPC::ResponseBuilder rb{ctx, 4};

    rb.Push(RESULT_SUCCESS);
    rb.Push<u64>(output_sz);

    NGLOG_DEBUG(Service_Audio, "called, buffer_size=0x{:X}", output_sz);
}

void AudRenU::GetAudioDevice(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};

    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<Audio::IAudioDevice>();

    NGLOG_DEBUG(Service_Audio, "called");
}

bool AudRenU::IsFeatureSupported(AudioFeatures feature, u32_le revision) const {
    u32_be version_num = (revision - Common::MakeMagic('R', 'E', 'V', '0')); // Byte swap
    switch (feature) {
    case AudioFeatures::Splitter:
        return version_num >= 2u;
    default:
        return false;
    }
}

} // namespace Service::Audio
