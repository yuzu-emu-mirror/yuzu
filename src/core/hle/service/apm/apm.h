// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace APM {

class APM final : public ServiceFramework<APM> {
public:
    explicit APM();
    ~APM();
};

/// Registers all AM services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace APM
} // namespace Service
