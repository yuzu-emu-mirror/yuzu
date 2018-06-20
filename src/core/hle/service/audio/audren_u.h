// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Kernel {
class HLERequestContext;
}

namespace Service::Audio {

class AudRenU final : public ServiceFramework<AudRenU> {
public:
    explicit AudRenU();
    ~AudRenU() = default;

private:
    void OpenAudioRenderer(Kernel::HLERequestContext& ctx);
    void GetAudioRendererWorkBufferSize(Kernel::HLERequestContext& ctx);
    void GetAudioDevice(Kernel::HLERequestContext& ctx);

    struct AudioRendererParameters {
        u32_le sample_rate;
        u32_le sample_count;
        u32_le unknown8;
        u32_le unknownC;
        u32_le voice_count;
        u32_le sink_count;
        u32_le effect_count;
        u32_le unknown1c;
        u8 unknown20;
        u8 padding1[3];
        u32_le splitter_count;
        u32_le unknown2c;
        u8 padding2[4];
        u32_le magic;
    };
    static_assert(sizeof(AudioRendererParameters) == 52,
                  "AudioRendererParameters is an invalid size");

    enum class AudioFeatures : u32 {
        Splitter,
    };

    bool IsFeatureSupported(AudioFeatures feature, u32_le revision) const;
};

} // namespace Service::Audio
