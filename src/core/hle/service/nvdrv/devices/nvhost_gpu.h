// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"

namespace Service {
namespace Nvidia {
namespace Devices {

class nvhost_gpu final : public nvdevice {
public:
    nvhost_gpu() = default;
    ~nvhost_gpu() override = default;

    u32 ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) override;

private:
    u32_le nvmap_fd{};
    u64_le user_data{};

    enum IoctlCommands {
        IocSetNVMAPfdCommand = 0x40044801,
        IocSetClientDataCommand = 0x40084714,
        IocGetClientDataCommand = 0x80084715,
    };

    struct set_nvmap_fd {
        u32_le nvmap_fd;
    };

    struct client_data {
        u64_le data;
    };

    u32 SetNVMAPfd(const std::vector<u8>& input, std::vector<u8>& output);
    u32 SetClientData(const std::vector<u8>& input, std::vector<u8>& output);
    u32 GetClientData(const std::vector<u8>& input, std::vector<u8>& output);
};

} // namespace Devices
} // namespace Nvidia
} // namespace Service
