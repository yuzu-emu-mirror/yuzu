// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <string>
#include "core/hle/kernel/event.h"
#include "core/hle/service/service.h"

namespace Service::Playreport {

class Playreport final : public ServiceFramework<Playreport> {
public:
    explicit Playreport(const char* name);
    ~Playreport() = default;

private:
    void SaveReportWithUser(Kernel::HLERequestContext& ctx);
};

void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace Service::Playreport
