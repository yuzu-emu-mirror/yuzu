// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include "core/hle/kernel/event.h"
#include "core/hle/service/service.h"

namespace Service::PlayReport {

class PlayReport final : public ServiceFramework<PlayReport> {
public:
    explicit PlayReport(const char* name);
    ~PlayReport() = default;

private:
    void SaveReportWithUser(Kernel::HLERequestContext& ctx);
};

void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace Service::PlayReport
