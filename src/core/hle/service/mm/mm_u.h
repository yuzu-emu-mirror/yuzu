// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/service.h"

namespace Service::MM {

class MM_U final : public ServiceFramework<MM_U> {
public:
    MM_U();
    ~MM_U() = default;

private:
    void Initialize(Kernel::HLERequestContext& ctx);
    void SetAndWait(Kernel::HLERequestContext& ctx);
    void Get(Kernel::HLERequestContext& ctx);

    u32 min{0};
    u32 max{0};
    u32 current{0};
};

/// Registers all MM services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace Service::MM
