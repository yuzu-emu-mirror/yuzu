// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/audio/audren_u.h"

namespace Service {
namespace Audio {

class IAudioRenderer final : public ServiceFramework<IAudioRenderer> {
public:
    IAudioRenderer() : ServiceFramework("IAudioRenderer") {
        static const FunctionInfo functions[] = {
            {0x0, nullptr, "GetAudioRendererSampleRate"},
            {0x1, nullptr, "GetAudioRendererSampleCount"},
            {0x2, nullptr, "GetAudioRendererMixBufferCount"},
            {0x3, nullptr, "GetAudioRendererState"},
            {0x4, nullptr, "RequestUpdateAudioRenderer"},
            {0x5, nullptr, "StartAudioRenderer"},
            {0x6, nullptr, "StopAudioRenderer"},
            {0x7, nullptr, "QuerySystemEvent"},
            {0x8, nullptr, "SetAudioRendererRenderingTimeLimit"},
            {0x9, nullptr, "GetAudioRendererRenderingTimeLimit"},
        };
        RegisterHandlers(functions);
    }
    ~IAudioRenderer() = default;
};

AudRenU::AudRenU() : ServiceFramework("audren:u") {
    static const FunctionInfo functions[] = {
        {0x00000000, nullptr, "OpenAudioRenderer"},
        {0x00000001, nullptr, "GetAudioRendererWorkBufferSize"},
        {0x00000002, nullptr, "GetAudioRenderersProcessMasterVolume"},
        {0x00000003, nullptr, "SetAudioRenderersProcessMasterVolume"},
    };
    RegisterHandlers(functions);
}

} // namespace Audio
} // namespace Service
