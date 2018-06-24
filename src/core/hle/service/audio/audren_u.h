// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Kernel {
class HLERequestContext;
}

namespace Service::Audio {

struct AudioRendererParameter {
    u32_le sample_rate;
    u32_le sample_count;
    u32_le unknown_8;
    u32_le unknown_c;
    u32_le voice_count;
    u32_le sink_count;
    u32_le effect_count;
    u32_le unknown_1c;
    u8 unknown_20;
    INSERT_PADDING_BYTES(3);
    u32_le splitter_count;
    u32_le unknown_2c;
    INSERT_PADDING_WORDS(1);
    u32_le revision;
};
static_assert(sizeof(AudioRendererParameter) == 52, "AudioRendererParameter is an invalid size");

class AudRenU final : public ServiceFramework<AudRenU> {
public:
    explicit AudRenU();
    ~AudRenU() = default;

private:
    void OpenAudioRenderer(Kernel::HLERequestContext& ctx);
    void GetAudioRendererWorkBufferSize(Kernel::HLERequestContext& ctx);
    void GetAudioDevice(Kernel::HLERequestContext& ctx);

    enum class AudioFeatures : u32 {
        Splitter,
    };

    bool IsFeatureSupported(AudioFeatures feature, u32_le revision) const;
};

} // namespace Service::Audio
