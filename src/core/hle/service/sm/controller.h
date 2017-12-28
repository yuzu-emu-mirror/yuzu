// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace SM {

class Controller final : public ServiceFramework<Controller> {
public:
    Controller();
    ~Controller() = default;

private:
    void ConvertSessionToDomain(Kernel::HLERequestContext& ctx);
    void QueryPointerBufferSize(Kernel::HLERequestContext& ctx);
};

} // namespace SM
} // namespace Service
