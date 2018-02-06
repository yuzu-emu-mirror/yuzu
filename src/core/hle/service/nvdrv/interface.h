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

class NVDRV final : public ServiceFramework<NVDRV> {
public:
    NVDRV(std::shared_ptr<Module> nvdrv, const char* name);
    ~NVDRV() = default;

private:
    void Open(Kernel::HLERequestContext& ctx);
    void Ioctl(Kernel::HLERequestContext& ctx);
    void Close(Kernel::HLERequestContext& ctx);
    void Initialize(Kernel::HLERequestContext& ctx);
    void QueryEvent(Kernel::HLERequestContext& ctx);
    void SetClientPID(Kernel::HLERequestContext& ctx);
    void FinishInitialize(Kernel::HLERequestContext& ctx);

    std::shared_ptr<Module> nvdrv;

    u64 pid{};
};

} // namespace Nvidia
} // namespace Service
