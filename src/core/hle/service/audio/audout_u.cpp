// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/audio/audout_u.h"

namespace Service {
namespace Audio {

class IAudioOut final : public ServiceFramework<IAudioOut> {
public:
    IAudioOut() : ServiceFramework("IAudioOut") {
        static const FunctionInfo functions[] = {
            {0x0, nullptr, "GetAudioOutState"},
            {0x1, nullptr, "StartAudioOut"},
            {0x2, nullptr, "StopAudioOut"},
            {0x3, nullptr, "AppendAudioOutBuffer_1"},
            {0x4, nullptr, "RegisterBufferEvent"},
            {0x5, nullptr, "GetReleasedAudioOutBuffer_1"},
            {0x6, nullptr, "ContainsAudioOutBuffer"},
            {0x7, nullptr, "AppendAudioOutBuffer_2"},
            {0x8, nullptr, "GetReleasedAudioOutBuffer_2"},
        };
        RegisterHandlers(functions);
    }
    ~IAudioOut() = default;
};

void AudOutU::ListAudioOuts(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::RequestParser rp{ctx};

    auto& buffer = ctx.BufferDescriptorB()[0];
    const std::string audio_interface = "AudioInterface";

    Memory::WriteBlock(buffer.Address(), &audio_interface[0], audio_interface.size());

    IPC::RequestBuilder rb = rp.MakeBuilder(3, 0, 0, 0);

    rb.Push(RESULT_SUCCESS);
    // TODO(st4rk): we're currently returning only one audio interface
    // (stringlist size)
    // however, it's highly possible to have more than one interface (despite that
    // libtransistor
    // requires only one).
    rb.Push<u32>(1);
}

AudOutU::AudOutU() : ServiceFramework("audout:u") {
    static const FunctionInfo functions[] = {{0x00000000, &AudOutU::ListAudioOuts, "ListAudioOuts"},
                                             {0x00000001, nullptr, "OpenAudioOut"},
                                             {0x00000002, nullptr, "Unknown2"},
                                             {0x00000003, nullptr, "Unknown3"}};
    RegisterHandlers(functions);
}

} // namespace Audio
} // namespace Service
