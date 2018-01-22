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

class AudOutU final : public ServiceFramework<AudOutU> {
public:
    AudOutU();
    ~AudOutU() = default;

private:
    void ListAudioOuts(Kernel::HLERequestContext& ctx);
    void OpenAudioOut(Kernel::HLERequestContext& ctx);
};

} // namespace Audio
} // namespace Service
