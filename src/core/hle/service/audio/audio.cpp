// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/audio/audin_u.h"
#include "core/hle/service/audio/audio.h"
#include "core/hle/service/audio/audout_u.h"
#include "core/hle/service/audio/audrec_u.h"
#include "core/hle/service/audio/audren_u.h"
#include "core/hle/service/audio/codecctl.h"
#include "core/hle/service/audio/hwopus.h"

namespace Service::Audio {

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<AudOutU>()->InstallAsService(service_manager);
    std::make_shared<AudInU>()->InstallAsService(service_manager);
    std::make_shared<AudRecU>()->InstallAsService(service_manager);
    std::make_shared<AudRenU>()->InstallAsService(service_manager);
    std::make_shared<CodecCtl>()->InstallAsService(service_manager);
    std::make_shared<HwOpus>()->InstallAsService(service_manager);
}

} // namespace Service::Audio
