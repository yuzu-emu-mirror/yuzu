// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"
#include <memory>

namespace Service {
namespace NVDRV {

class NVDRV_A final : public ServiceFramework<NVDRV_A> {
public:
    NVDRV_A();
    ~NVDRV_A() = default;

private:
    void Open(Kernel::HLERequestContext& ctx);
    void Ioctl(Kernel::HLERequestContext& ctx);
    void Initialize(Kernel::HLERequestContext& ctx);

    u32 next_fd = 1;

    std::unordered_map<u32, std::shared_ptr<nvdevice>> open_files;
    std::unordered_map<std::string, std::shared_ptr<nvdevice>> devices;
};

} // namespace NVDRV
} // namespace Service
