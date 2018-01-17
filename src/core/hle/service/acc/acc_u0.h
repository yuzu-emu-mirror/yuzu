// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace Account {

class ACC_U0 final : public ServiceFramework<ACC_U0> {
public:
    ACC_U0();
    ~ACC_U0() = default;

private:
    void InitializeApplicationInfo(Kernel::HLERequestContext& ctx);
};

} // namespace Account
} // namespace Service
