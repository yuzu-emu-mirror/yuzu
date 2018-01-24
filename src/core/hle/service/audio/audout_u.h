// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Kernel {
class HLERequestContext;
}

namespace Service {
namespace Audio {

class IAudioOut;

class AudOutU final : public ServiceFramework<AudOutU> {
public:
    AudOutU();
    ~AudOutU() = default;

private:
    std::shared_ptr<IAudioOut> audio_out_interface;

    void ListAudioOuts(Kernel::HLERequestContext& ctx);
    void OpenAudioOut(Kernel::HLERequestContext& ctx);

    enum class PcmFormat : u32 {
        Invalid = 0,
        Int8 = 1,
        Int16 = 2,
        Int24 = 3,
        Int32 = 4,
        PcmFloat = 5,
        Adpcm = 6,
    };
};

} // namespace Audio
} // namespace Service
