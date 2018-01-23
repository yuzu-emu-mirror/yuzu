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

    enum PCM_FORMAT {
        INVALID,
        INT8,
        INT16,
        INT24,
        INT32,
        PCM_FLOAT,
        ADPCM
    };
};

} // namespace Audio
} // namespace Service
