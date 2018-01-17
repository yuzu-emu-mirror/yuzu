// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/service.h"

namespace Service {
namespace Sfdnsres {

class Sfdnsres final : public ServiceFramework<Sfdnsres> {
public:
    Sfdnsres();
    ~Sfdnsres() = default;

private:
};

void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace Sfdnsres
} // namespace Service