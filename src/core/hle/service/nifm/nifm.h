// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service::NIFM {

class Module final {
public:
    class Interface : public ServiceFramework<Interface> {
    public:
        explicit Interface(std::shared_ptr<Module> module, const char* name);

        void CreateGeneralServiceOld(Kernel::HLERequestContext& ctx);
        void CreateGeneralService(Kernel::HLERequestContext& ctx);

    protected:
        std::shared_ptr<Module> module;
    };
};

void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace Service::NIFM
