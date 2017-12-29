// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace PCTL {

/// Registers all PCTL services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace PCTL
} // namespace Service
