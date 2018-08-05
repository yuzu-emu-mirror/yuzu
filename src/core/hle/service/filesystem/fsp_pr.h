// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service::FileSystem {

class FSP_PR final : public ServiceFramework<FSP_PR> {
public:
    explicit FSP_PR();
};

} // namespace Service::FileSystem
