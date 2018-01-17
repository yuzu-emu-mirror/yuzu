// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/sfdnsres/sfdnsres.h"

namespace Service {
namespace Sfdnsres {

Sfdnsres::Sfdnsres() : ServiceFramework("sfdnsres") {
    /*static const FunctionInfo functions[] = {
    };
    RegisterHandlers(functions);*/
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<Sfdnsres>()->InstallAsService(service_manager);
}

} // namespace Sfdnsres
} // namespace Service