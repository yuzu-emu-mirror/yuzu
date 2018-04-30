// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/core_timing.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/audio/audren_u.h"

namespace Service::Audio {

/// TODO(bunnei): Find a proper value for the audio_ticks
constexpr u64 audio_ticks{static_cast<u64>(BASE_CLOCK_RATE / 200)};

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
        NGLOG_DEBUG(Service_Audio, "{}", ctx.Description());
        AudioRendererResponseData response_data{};

        response_data.section_0_size =
            static_cast<u32>(response_data.state_entries.size() * sizeof(AudioRendererStateEntry));
        response_data.section_1_size = static_cast<u32>(response_data.section_1.size());
        response_data.section_2_size = static_cast<u32>(response_data.section_2.size());
        response_data.section_3_size = static_cast<u32>(response_data.section_3.size());
        response_data.section_4_size = static_cast<u32>(response_data.section_4.size());
        response_data.section_5_size = static_cast<u32>(response_data.section_5.size());
        response_data.total_size = sizeof(AudioRendererResponseData);

        for (unsigned i = 0; i < response_data.state_entries.size(); i++) {
            // 4 = Busy and 5 = Ready?
            response_data.state_entries[i].state = 5;
        }

        ctx.WriteBuffer(&response_data, response_data.total_size);

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

    struct AudioRendererStateEntry {
        u32_le state;
        u32_le unknown_4;
        u32_le unknown_8;
        u32_le unknown_c;
    };
    static_assert(sizeof(AudioRendererStateEntry) == 0x10,
                  "AudioRendererStateEntry has wrong size");

    struct AudioRendererResponseData {
        u32_le unknown_0;
        u32_le section_5_size;
        u32_le section_0_size;
        u32_le section_1_size;
        u32_le unknown_10;
        u32_le section_2_size;
        u32_le unknown_18;
        u32_le section_3_size;
        u32_le section_4_size;
        u32_le unknown_24;
        u32_le unknown_28;
        u32_le unknown_2c;
        u32_le unknown_30;
        u32_le unknown_34;
        u32_le unknown_38;
        u32_le total_size;

        std::array<AudioRendererStateEntry, 0x18e> state_entries;

        std::array<u8, 0x600> section_1;
        std::array<u8, 0xe0> section_2;
        std::array<u8, 0x20> section_3;
        std::array<u8, 0x10> section_4;
        std::array<u8, 0xb0> section_5;
    };
    static_assert(sizeof(AudioRendererResponseData) == 0x20e0,
                  "AudioRendererResponseData has wrong size");

    /// This is used to trigger the audio event callback.
    CoreTiming::EventType* audio_event;

    Kernel::SharedPtr<Kernel::Event> system_event;
};

class IAudioDevice final : public ServiceFramework<IAudioDevice> {
public:
    IAudioDevice() : ServiceFramework("IAudioDevice") {
        static const FunctionInfo functions[] = {
            {0x0, &IAudioDevice::ListAudioDeviceName, "ListAudioDeviceName"},
            {0x1, &IAudioDevice::SetAudioDeviceOutputVolume, "SetAudioDeviceOutputVolume"},
            {0x2, nullptr, "GetAudioDeviceOutputVolume"},
            {0x3, &IAudioDevice::GetActiveAudioDeviceName, "GetActiveAudioDeviceName"},
            {0x4, &IAudioDevice::QueryAudioDeviceSystemEvent, "QueryAudioDeviceSystemEvent"},
            {0x5, &IAudioDevice::GetActiveChannelCount, "GetActiveChannelCount"},
            {0x6, &IAudioDevice::ListAudioDeviceName,
             "ListAudioDeviceNameAuto"}, // TODO(ogniK): Confirm if autos are identical to non auto
            {0x7, &IAudioDevice::SetAudioDeviceOutputVolume, "SetAudioDeviceOutputVolumeAuto"},
            {0x8, nullptr, "GetAudioDeviceOutputVolumeAuto"},
            {0xa, &IAudioDevice::GetActiveAudioDeviceName, "GetActiveAudioDeviceNameAuto"},
            {0xb, nullptr, "QueryAudioDeviceInputEvent"},
            {0xc, nullptr, "QueryAudioDeviceOutputEvent"}};
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
    IPC::ResponseBuilder rb{ctx, 4};

    rb.Push(RESULT_SUCCESS);
    rb.Push<u64>(0x4000);

    NGLOG_WARNING(Service_Audio, "(STUBBED) called");
}

void AudRenU::GetAudioDevice(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};

    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<Audio::IAudioDevice>();

    NGLOG_DEBUG(Service_Audio, "called");
}

} // namespace Service::Audio
