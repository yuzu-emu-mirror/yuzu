// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/apm/apm.h"

namespace Service {
namespace APM {

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<APM>()->InstallAsService(service_manager);
}

APM::APM() : ServiceFramework("apm") {
    static const FunctionInfo functions[] = {
        {0x00000000, nullptr, "OpenSession"},
        {0x00000001, nullptr, "GetPerformanceMode"},
    };
    RegisterHandlers(functions);
}

APM::~APM() = default;

} // namespace APM
} // namespace Service
