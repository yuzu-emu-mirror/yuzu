// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/event.h"
#include "core/hle/service/service.h"

namespace Service {
namespace VI {

class IApplicationDisplayService final : public ServiceFramework<IApplicationDisplayService> {
public:
    IApplicationDisplayService();
    ~IApplicationDisplayService() = default;

private:
    void GetRelayService(Kernel::HLERequestContext& ctx);
    void GetSystemDisplayService(Kernel::HLERequestContext& ctx);
    void GetManagerDisplayService(Kernel::HLERequestContext& ctx);
    void OpenDisplay(Kernel::HLERequestContext& ctx);
    void SetLayerScalingMode(Kernel::HLERequestContext& ctx);
    void OpenLayer(Kernel::HLERequestContext& ctx);
    void GetDisplayVsyncEvent(Kernel::HLERequestContext& ctx);

    Kernel::SharedPtr<Kernel::Event> vsync_event;
};

/// Registers all VI services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace VI
} // namespace Service
