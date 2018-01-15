// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/service.h"

namespace Service {
namespace Audio {

class AudOutU final : public ServiceFramework<AudOutU> {
public:
    AudOutU();
    ~AudOutU() = default;

private:
    void ListAudioOuts(Kernel::HLERequestContext& ctx);
};

} // namespace Audio
} // namespace Service
