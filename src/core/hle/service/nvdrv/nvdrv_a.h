// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/service.h"

namespace Service {
namespace NVDRV {

namespace Devices {
class nvdevice;
}

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

    /// Id to use for the next open file descriptor.
    u32 next_fd = 1;

    /// Mapping of file descriptors to the devices they reference.
    std::unordered_map<u32, std::shared_ptr<Devices::nvdevice>> open_files;

    /// Mapping of device node names to their implementation.
    std::unordered_map<std::string, std::shared_ptr<Devices::nvdevice>> devices;
};

extern std::weak_ptr<NVDRV_A> nvdrv_a;

} // namespace NVDRV
} // namespace Service
