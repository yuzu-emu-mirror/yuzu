// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/service.h"

namespace Service {
namespace NIFM {

class NIFM_A final : public ServiceFramework<NIFM_A> {
public:
    NIFM_A();
    ~NIFM_A() = default;

private:
    void CreateGeneralServiceOld(Kernel::HLERequestContext& ctx);
    void CreateGeneralService(Kernel::HLERequestContext& ctx);
};

} // namespace NIFM
} // namespace Service
