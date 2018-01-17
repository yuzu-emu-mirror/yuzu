// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/bsd/bsd.h"
#include "core/hle/service/bsd/bsd_u.h"

namespace Service {
namespace BSD {

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<BSD_U>()->InstallAsService(service_manager);
}

} // namespace BSD
} // namespace Service
