// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/aoc/aoc_u.h"

namespace Service {
namespace AOC {

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<AOC_U>()->InstallAsService(service_manager);
}

AOC_U::AOC_U() : ServiceFramework("aoc:u") {}

} // namespace AOC
} // namespace Service
