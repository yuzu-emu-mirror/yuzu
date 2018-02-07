// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <boost/optional.hpp>
#include "core/hle/kernel/event.h"
#include "core/hle/service/nvflinger/nvflinger.h"
#include "core/hle/service/service.h"

namespace CoreTiming {
struct EventType;
}

namespace Service {
namespace VI {

class IApplicationDisplayService final : public ServiceFramework<IApplicationDisplayService> {
public:
    IApplicationDisplayService(std::shared_ptr<NVFlinger::NVFlinger> nv_flinger);
    ~IApplicationDisplayService() = default;

private:
    void GetRelayService(Kernel::HLERequestContext& ctx);
    void GetSystemDisplayService(Kernel::HLERequestContext& ctx);
    void GetManagerDisplayService(Kernel::HLERequestContext& ctx);
    void GetIndirectDisplayTransactionService(Kernel::HLERequestContext& ctx);
    void OpenDisplay(Kernel::HLERequestContext& ctx);
    void CloseDisplay(Kernel::HLERequestContext& ctx);
    void SetLayerScalingMode(Kernel::HLERequestContext& ctx);
    void ListDisplays(Kernel::HLERequestContext& ctx);
    void OpenLayer(Kernel::HLERequestContext& ctx);
    void CreateStrayLayer(Kernel::HLERequestContext& ctx);
    void DestroyStrayLayer(Kernel::HLERequestContext& ctx);
    void GetDisplayVsyncEvent(Kernel::HLERequestContext& ctx);

    std::shared_ptr<NVFlinger::NVFlinger> nv_flinger;
};

/// Registers all VI services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager,
                       std::shared_ptr<NVFlinger::NVFlinger> nv_flinger);

} // namespace VI
} // namespace Service
