// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/nifm/nifm.h"

namespace Service {
namespace NIFM {

class NIFM_U final : public Module::Interface {
public:
    explicit NIFM_U(std::shared_ptr<Module> module);
};

} // namespace NIFM
} // namespace Service
