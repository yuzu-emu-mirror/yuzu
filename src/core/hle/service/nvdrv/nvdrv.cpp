// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/nvdrv/nvdrv_a.h"

namespace Service {
namespace NVDRV {

std::weak_ptr<NVDRV_A> nvdrv_a;

void InstallInterfaces(SM::ServiceManager& service_manager) {
    auto nvdrv = std::make_shared<NVDRV_A>();
    nvdrv->InstallAsService(service_manager);
    nvdrv_a = nvdrv;
}

} // namespace NVDRV
} // namespace Service
