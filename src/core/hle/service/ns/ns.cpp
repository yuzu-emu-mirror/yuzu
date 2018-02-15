// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/ns/ns.h"
#include "core/hle/service/ns/pl_u.h"

namespace Service {
namespace NS {

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<PL_U>()->InstallAsService(service_manager);
}

} // namespace NS
} // namespace Service
