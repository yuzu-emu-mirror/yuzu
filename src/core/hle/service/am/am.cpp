// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/am/am.h"
#include "core/hle/service/am/applet_oe.h"

namespace Service {
namespace AM {

void InstallInterfaces(SM::ServiceManager& service_manager,
                       std::shared_ptr<NVFlinger::NVFlinger> nvflinger) {
    std::make_shared<AppletOE>(nvflinger)->InstallAsService(service_manager);
}

} // namespace AM
} // namespace Service
