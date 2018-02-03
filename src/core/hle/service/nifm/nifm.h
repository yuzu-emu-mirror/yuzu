// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace NIFM {

class IGeneralService final : public ServiceFramework<IGeneralService> {
public:
    IGeneralService();

private:
    void GetClientId(Kernel::HLERequestContext& ctx);
    void CreateScanRequest(Kernel::HLERequestContext& ctx);
    void CreateRequest(Kernel::HLERequestContext& ctx);
    void RemoveNetworkProfile(Kernel::HLERequestContext& ctx);
    void CreateTemporaryNetworkProfile(Kernel::HLERequestContext& ctx);
};

void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace NIFM
} // namespace Service
