// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/nifm/nifm_s.h"

namespace Service::NIFM {

NIFM_S::NIFM_S(std::shared_ptr<Module> module) : Module::Interface(std::move(module), "nifm:s") {
    static const FunctionInfo functions[] = {
        {4, &NIFM_S::CreateGeneralServiceOld, "CreateGeneralServiceOld"},
        {5, &NIFM_S::CreateGeneralService, "CreateGeneralService"},
    };
    RegisterHandlers(functions);
}

} // namespace Service::NIFM
