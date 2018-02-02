// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/service.h"

namespace Service {
namespace NIFM {

class NIFM_S final : public ServiceFramework<NIFM_S> {
public:
    NIFM_S();
    ~NIFM_S() = default;

private:
    void CreateGeneralServiceOld(Kernel::HLERequestContext& ctx);
    void CreateGeneralService(Kernel::HLERequestContext& ctx);
};

} // namespace NIFM
} // namespace Service
