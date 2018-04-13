// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace Nvidia {

class NVMEMP final : public ServiceFramework<NVMEMP> {
public:
    NVMEMP();
    ~NVMEMP() = default;

private:
    void Cmd0(Kernel::HLERequestContext& ctx);
    void Cmd1(Kernel::HLERequestContext& ctx);
};

} // namespace Nvidia
} // namespace Service
