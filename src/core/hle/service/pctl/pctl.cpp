// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/pctl/pctl.h"
#include "core/hle/service/pctl/pctl_a.h"

namespace Service::PCTL {

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<PCTL_A>()->InstallAsService(service_manager);
}

} // namespace Service::PCTL
