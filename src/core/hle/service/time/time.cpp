// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/time/time.h"
#include "core/hle/service/time/time_s.h"

namespace Service {
namespace Time {

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<TimeS>()->InstallAsService(service_manager);
}

} // namespace Time
} // namespace Service
