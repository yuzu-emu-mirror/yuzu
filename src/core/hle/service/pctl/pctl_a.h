// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service::PCTL {

class PCTL_A final : public ServiceFramework<PCTL_A> {
public:
    PCTL_A();
    ~PCTL_A() = default;

private:
    void CreateService(Kernel::HLERequestContext& ctx);
};

} // namespace Service::PCTL
