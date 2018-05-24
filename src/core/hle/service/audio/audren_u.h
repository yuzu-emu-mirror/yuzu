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

    struct WorkerBufferParameters {
        u32_le sampleRate;
        u32_le sampleCount;
        u32_le Unknown8;
        u32_le UnknownC;
        u32_le voiceCount;
        u32_le sinkCount;
        u32_le effectCount;
        u32_le Unknown1C;
        u8 Unknown20;
        u8 padding1[3];
        u32_le splitterCount;
        u32_le Unknown2C;
        u8 padding2[4];
        u32_le MAGIC;
    };
    static_assert(sizeof(WorkerBufferParameters) == 52,
                  "WorkerBufferParameters is an invalid size");

    enum class AudioFeatures : u32 {
        UNKNOWN = 0, // Placeholder
        Splitter,
    };

    bool IsFeatureSupported(AudioFeatures feature, u32_le Revision);
};

} // namespace Service::Audio
