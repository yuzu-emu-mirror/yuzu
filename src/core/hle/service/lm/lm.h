// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/service.h"

namespace Service::LM {

class LM final : public ServiceFramework<LM> {
public:
    LM();
    ~LM() = default;

private:
    void OpenLogger(Kernel::HLERequestContext& ctx);
};

/// Registers all LM services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace Service::LM
