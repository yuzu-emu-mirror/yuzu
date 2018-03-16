// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/nifm/nifm_u.h"

namespace Service {
namespace NIFM {

NIFM_U::NIFM_U(std::shared_ptr<Module> module) : Module::Interface(std::move(module), "nifm:u") {
    static const FunctionInfo functions[] = {
        {4, &NIFM_U::CreateGeneralServiceOld, "CreateGeneralServiceOld"},
        {5, &NIFM_U::CreateGeneralService, "CreateGeneralService"},
    };
    RegisterHandlers(functions);
}

} // namespace NIFM
} // namespace Service
