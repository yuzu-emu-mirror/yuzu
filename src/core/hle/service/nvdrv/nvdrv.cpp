// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/nvdrv/nvdrv_a.h"

namespace Service {
namespace NVDRV {

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<NVDRV_A>()->InstallAsService(service_manager);
}

} // namespace nvdrv
} // namespace Service
