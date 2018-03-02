// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace Set {

class SET_SYS final : public ServiceFramework<SET_SYS> {
public:
    explicit SET_SYS();
    ~SET_SYS() = default;

private:
    void GetColorSetId(Kernel::HLERequestContext& ctx);
};

} // namespace Set
} // namespace Service
