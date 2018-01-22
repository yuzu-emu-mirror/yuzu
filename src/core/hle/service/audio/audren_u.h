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

class AudRenU final : public ServiceFramework<AudRenU> {
public:
    explicit AudRenU();
    ~AudRenU() = default;
};

} // namespace Audio
} // namespace Service
