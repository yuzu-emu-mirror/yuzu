// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace AOC {

class AOC_U final : public ServiceFramework<AOC_U> {
public:
    AOC_U();
    ~AOC_U() = default;
};

/// Registers all AOC services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace AOC
} // namespace Service
