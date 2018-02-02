// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "core/hle/service/service.h"

namespace Service {
namespace NVFlinger {
class NVFlinger;
}

namespace VI {

class VI_U final : public ServiceFramework<VI_U> {
public:
    VI_U(std::shared_ptr<NVFlinger::NVFlinger> nv_flinger);
    ~VI_U() = default;

private:
    void GetDisplayService(Kernel::HLERequestContext& ctx);

    std::shared_ptr<NVFlinger::NVFlinger> nv_flinger;
};

} // namespace VI
} // namespace Service
