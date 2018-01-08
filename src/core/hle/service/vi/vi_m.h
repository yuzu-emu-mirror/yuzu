// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "core/hle/service/service.h"

namespace Service {
namespace VI {

class VI_M final : public ServiceFramework<VI_M> {
public:
    VI_M();
    ~VI_M() = default;

private:
    void GetDisplayService(Kernel::HLERequestContext& ctx);

    std::shared_ptr<NVFlinger> nv_flinger;
};

} // namespace VI
} // namespace Service
