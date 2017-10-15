// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace LM {

class LM final : public ServiceFramework<LM> {
public:
    explicit LM();
    ~LM();

private:
    void Initialize(Kernel::HLERequestContext& ctx);
};

/// Registers all LM services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace LM
} // namespace Service
