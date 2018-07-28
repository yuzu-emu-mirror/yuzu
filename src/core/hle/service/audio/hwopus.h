// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <opus.h>
#include "core/hle/service/service.h"

namespace Service::Audio {

class HwOpus final : public ServiceFramework<HwOpus> {
public:
    explicit HwOpus();
    ~HwOpus() = default;

private:
    void OpenOpusDecoder(Kernel::HLERequestContext& ctx);
    void GetWorkBufferSize(Kernel::HLERequestContext& ctx);
    size_t WorkerBufferSize(u32 channel_count);
};

} // namespace Service::Audio
