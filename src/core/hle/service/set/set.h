// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service::Set {

class SET final : public ServiceFramework<SET> {
public:
    explicit SET();
    ~SET() = default;

private:
    void GetAvailableLanguageCodes(Kernel::HLERequestContext& ctx);
};

} // namespace Service::Set
