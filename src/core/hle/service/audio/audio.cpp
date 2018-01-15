// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/ipc_helpers.h"
#include "core/hle/service/audio/audio.h"
#include "core/hle/service/audio/audout_u.h"

namespace Service {
namespace Audio {

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<AudOutU>()->InstallAsService(service_manager);
}

} // namespace Audio
} // namespace Service
