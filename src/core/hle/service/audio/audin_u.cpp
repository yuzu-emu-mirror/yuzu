// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/audio/audin_u.h"

namespace Service {
namespace Audio {

class IAudioIn final : public ServiceFramework<IAudioIn> {
public:
    IAudioIn() : ServiceFramework("IAudioIn") {
        static const FunctionInfo functions[] = {
            {0x0, nullptr, "GetAudioInState"},
            {0x1, nullptr, "StartAudioIn"},
            {0x2, nullptr, "StopAudioIn"},
            {0x3, nullptr, "AppendAudioInBuffer_1"},
            {0x4, nullptr, "RegisterBufferEvent"},
            {0x5, nullptr, "GetReleasedAudioInBuffer_1"},
            {0x6, nullptr, "ContainsAudioInBuffer"},
            {0x7, nullptr, "AppendAudioInBuffer_2"},
            {0x8, nullptr, "GetReleasedAudioInBuffer_2"},
        };
        RegisterHandlers(functions);
    }
    ~IAudioIn() = default;
};

AudInU::AudInU() : ServiceFramework("audin:u") {
    static const FunctionInfo functions[] = {
        {0x00000000, nullptr, "ListAudioIns"},
        {0x00000001, nullptr, "OpenAudioIn"},
    };
    RegisterHandlers(functions);
}

} // namespace Audio
} // namespace Service
