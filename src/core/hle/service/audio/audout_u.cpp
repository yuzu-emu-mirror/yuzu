// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vector>
#include "common/logging/log.h"
#include "core/core_timing.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/audio/audout_u.h"

namespace Service::Audio {

/// Switch sample rate frequency
constexpr u32 sample_rate{48000};
/// TODO(st4rk): dynamic number of channels, as I think Switch has support
/// to more audio channels (probably when Docked I guess)
constexpr u32 audio_channels{2};
/// TODO(st4rk): find a proper value for the audio_ticks
constexpr u64 audio_ticks{static_cast<u64>(CoreTiming::BASE_CLOCK_RATE / 500)};

class IAudioOut final : public ServiceFramework<IAudioOut> {
public:
    IAudioOut() : ServiceFramework("IAudioOut"), audio_out_state(AudioState::Stopped) {
        static const FunctionInfo functions[] = {
            {0, &IAudioOut::GetAudioOutState, "GetAudioOutState"},
            {1, &IAudioOut::StartAudioOut, "StartAudioOut"},
            {2, &IAudioOut::StopAudioOut, "StopAudioOut"},
            {3, &IAudioOut::AppendAudioOutBufferImpl, "AppendAudioOutBuffer"},
            {4, &IAudioOut::RegisterBufferEvent, "RegisterBufferEvent"},
            {5, &IAudioOut::GetReleasedAudioOutBufferImpl, "GetReleasedAudioOutBuffer"},
            {6, nullptr, "ContainsAudioOutBuffer"},
            {7, &IAudioOut::AppendAudioOutBufferImpl, "AppendAudioOutBufferAuto"},
            {8, &IAudioOut::GetReleasedAudioOutBufferImpl, "GetReleasedAudioOutBufferAuto"},
            {9, nullptr, "GetAudioOutBufferCount"},
            {10, nullptr, "GetAudioOutPlayedSampleCount"},
            {11, nullptr, "FlushAudioOutBuffers"},
        };
        RegisterHandlers(functions);

        // This is the event handle used to check if the audio buffer was released
        buffer_event =
            Kernel::Event::Create(Kernel::ResetType::OneShot, "IAudioOutBufferReleasedEvent");

        // Register event callback to update the Audio Buffer
        audio_event = CoreTiming::RegisterEvent(
            "IAudioOut::UpdateAudioBuffersCallback", [this](u64 userdata, int cycles_late) {
                UpdateAudioBuffersCallback();
                CoreTiming::ScheduleEvent(audio_ticks - cycles_late, audio_event);
            });

        // Start the audio event
        CoreTiming::ScheduleEvent(audio_ticks, audio_event);
    }

    ~IAudioOut() {
        CoreTiming::UnscheduleEvent(audio_event, 0);
    }

private:
    void GetAudioOutState(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push(static_cast<u32>(audio_out_state));
    }

    void StartAudioOut(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_Audio, "(STUBBED) called");

        // Start audio
        audio_out_state = AudioState::Started;

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void StopAudioOut(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_Audio, "(STUBBED) called");

        // Stop audio
        audio_out_state = AudioState::Stopped;

        queue_keys.clear();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void RegisterBufferEvent(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_Audio, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(buffer_event);
    }

    void AppendAudioOutBufferImpl(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_Audio, "(STUBBED) called");
        IPC::RequestParser rp{ctx};

        const u64 key{rp.Pop<u64>()};
        queue_keys.insert(queue_keys.begin(), key);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }

    void GetReleasedAudioOutBufferImpl(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_Audio, "(STUBBED) called");

        // TODO(st4rk): This is how libtransistor currently implements the
        // GetReleasedAudioOutBuffer, it should return the key (a VAddr) to the app and this address
        // is used to know which buffer should be filled with data and send again to the service
        // through AppendAudioOutBuffer. Check if this is the proper way to do it.
        u64 key{0};

        if (queue_keys.size()) {
            key = queue_keys.back();
            queue_keys.pop_back();
        }

        ctx.WriteBuffer(&key, sizeof(u64));

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        // TODO(st4rk): This might be the total of released buffers, needs to be verified on
        // hardware
        rb.Push<u32>(static_cast<u32>(queue_keys.size()));
    }

    void UpdateAudioBuffersCallback() {
        if (audio_out_state != AudioState::Started) {
            return;
        }

        if (queue_keys.empty()) {
            return;
        }

        buffer_event->Signal();
    }

    enum class AudioState : u32 {
        Started,
        Stopped,
    };

    /// This is used to trigger the audio event callback that is going to read the samples from the
    /// audio_buffer list and enqueue the samples using the sink (audio_core).
    CoreTiming::EventType* audio_event;

    /// This is the evend handle used to check if the audio buffer was released
    Kernel::SharedPtr<Kernel::Event> buffer_event;

    /// (st4rk): This is just a temporary workaround for the future implementation. Libtransistor
    /// uses the key as an address in the App, so we need to return when the
    /// GetReleasedAudioOutBuffer_1 is called, otherwise we'll run in problems, because
    /// libtransistor uses the key returned as an pointer.
    std::vector<u64> queue_keys;

    AudioState audio_out_state;
};

void AudOutU::ListAudioOutsImpl(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_Audio, "(STUBBED) called");
    IPC::RequestParser rp{ctx};

    const std::string audio_interface = "AudioInterface";
    ctx.WriteBuffer(audio_interface.c_str(), audio_interface.size());

    IPC::ResponseBuilder rb = rp.MakeBuilder(3, 0, 0);

    rb.Push(RESULT_SUCCESS);
    // TODO(st4rk): We're currently returning only one audio interface (stringlist size). However,
    // it's highly possible to have more than one interface (despite that libtransistor requires
    // only one).
    rb.Push<u32>(1);
}

void AudOutU::OpenAudioOutImpl(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_Audio, "(STUBBED) called");

    if (!audio_out_interface) {
        audio_out_interface = std::make_shared<IAudioOut>();
    }

    IPC::ResponseBuilder rb{ctx, 6, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(sample_rate);
    rb.Push<u32>(audio_channels);
    rb.Push<u32>(static_cast<u32>(PcmFormat::Int16));
    rb.Push<u32>(0); // This field is unknown
    rb.PushIpcInterface<Audio::IAudioOut>(audio_out_interface);
}

AudOutU::AudOutU() : ServiceFramework("audout:u") {
    static const FunctionInfo functions[] = {{0, &AudOutU::ListAudioOutsImpl, "ListAudioOuts"},
                                             {1, &AudOutU::OpenAudioOutImpl, "OpenAudioOut"},
                                             {2, &AudOutU::ListAudioOutsImpl, "ListAudioOutsAuto"},
                                             {3, &AudOutU::OpenAudioOutImpl, "OpenAudioOutAuto"}};
    RegisterHandlers(functions);
}

} // namespace Service::Audio
