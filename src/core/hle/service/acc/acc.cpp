// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/acc/acc.h"
#include "core/hle/service/acc/acc_u0.h"

namespace Service {
namespace Account {

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<ACC_U0>()->InstallAsService(service_manager);
}

} // namespace Account
} // namespace Service
