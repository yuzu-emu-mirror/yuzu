// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/nifm/nifm.h"

namespace Service::NIFM {

class NIFM_S final : public Module::Interface {
public:
    explicit NIFM_S(std::shared_ptr<Module> module);
};

} // namespace Service::NIFM
