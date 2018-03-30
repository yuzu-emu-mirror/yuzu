// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/nfp/nfp_user.h"

namespace Service {
namespace NFP {

NFP_User::NFP_User(std::shared_ptr<Module> module)
    : Module::Interface(std::move(module), "nfp:user") {
    static const FunctionInfo functions[] = {
        {0, &NFP_User::Unknown, "Unknown"},
    };
    RegisterHandlers(functions);
}

} // namespace NFP
} // namespace Service
