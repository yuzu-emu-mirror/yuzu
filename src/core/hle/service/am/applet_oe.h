// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace AM {

class AppletOE final : public ServiceFramework<AppletOE> {
public:
    explicit AppletOE();
    ~AppletOE();
};

} // namespace AM
} // namespace Service
