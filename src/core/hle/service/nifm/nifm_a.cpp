// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/nifm/nifm_a.h"

namespace Service {
namespace NIFM {

NIFM_A::NIFM_A(std::shared_ptr<Module> module) : Module::Interface(std::move(module), "nifm:a") {
    static const FunctionInfo functions[] = {
        {4, &NIFM_A::CreateGeneralServiceOld, "CreateGeneralServiceOld"},
        {5, &NIFM_A::CreateGeneralService, "CreateGeneralService"},
    };
    RegisterHandlers(functions);
}

} // namespace NIFM
} // namespace Service
