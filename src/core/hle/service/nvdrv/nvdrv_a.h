// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "core/hle/service/service.h"
#include "core/hle/service/nvdrv/nvdrv.h"

namespace Service {
namespace NVDRV {

class NVDRV_A final : public ServiceFramework<NVDRV_A> {
public:
    NVDRV_A();
    ~NVDRV_A() = default;

    /// Returns a pointer to one of the available devices, identified by its name.
    template <typename T>
    std::shared_ptr<T> GetDevice(std::string name) {
        auto itr = devices.find(name);
        if (itr == devices.end())
            return nullptr;
        return std::static_pointer_cast<T>(itr->second);
    }

private:
    void Open(Kernel::HLERequestContext& ctx);
    void Ioctl(Kernel::HLERequestContext& ctx);
    void Initialize(Kernel::HLERequestContext& ctx);

    u32 next_fd = 1;

    std::unordered_map<u32, std::shared_ptr<nvdevice>> open_files;
    std::unordered_map<std::string, std::shared_ptr<nvdevice>> devices;
};

extern std::weak_ptr<NVDRV_A> nvdrv_a;

} // namespace NVDRV
} // namespace Service
