// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/service.h"

namespace Service {
namespace LM {

class LM final : public ServiceFramework<LM> {
public:
    LM();
    ~LM() = default;

private:
    void Initialize(Kernel::HLERequestContext& ctx);

    std::vector<Kernel::SharedPtr<Kernel::ClientPort>> registered_loggers;
};

/// Registers all LM services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace LM
} // namespace Service
