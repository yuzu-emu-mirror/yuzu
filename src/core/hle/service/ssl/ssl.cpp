// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/ssl/ssl.h"

namespace Service {
namespace SSL {

SSL::SSL() : ServiceFramework("ssl") {}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<SSL>()->InstallAsService(service_manager);
}

} // namespace SSL
} // namespace Service
