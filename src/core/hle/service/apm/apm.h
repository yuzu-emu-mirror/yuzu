// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace APM {

enum class PerformanceMode : u8 {
    Handheld = 0,
    Docked = 1,
};

class APM final : public ServiceFramework<APM> {
public:
    APM();
    ~APM() = default;

private:
    void OpenSession(Kernel::HLERequestContext& ctx);
};

/// Registers all AM services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace APM
} // namespace Service
