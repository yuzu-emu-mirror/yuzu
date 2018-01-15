// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/service.h"

namespace Service {
namespace Nvidia {

class NVDRV_A final : public ServiceFramework<NVDRV_A> {
public:
    NVDRV_A(std::shared_ptr<Module> nvdrv);
    ~NVDRV_A() = default;

private:
    void Open(Kernel::HLERequestContext& ctx);
    void Ioctl(Kernel::HLERequestContext& ctx);
    void Initialize(Kernel::HLERequestContext& ctx);

    std::shared_ptr<Module> nvdrv;
};

} // namespace Nvidia
} // namespace Service
