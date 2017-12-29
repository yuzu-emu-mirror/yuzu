// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace PCTL {

class PCTL_A final : public ServiceFramework<PCTL_A> {
public:
    PCTL_A();
    ~PCTL_A() = default;

private:
    void GetService(Kernel::HLERequestContext& ctx);
};

} // namespace PCTL
} // namespace Service
