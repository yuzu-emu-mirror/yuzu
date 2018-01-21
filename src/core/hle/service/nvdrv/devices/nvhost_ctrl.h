// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "common/common_types.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"

namespace Service {
namespace Nvidia {
namespace Devices {

class nvhost_ctrl final : public nvdevice {
public:
    nvhost_ctrl() = default;
    ~nvhost_ctrl() override = default;

    u32 ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) override;

private:
    enum IoctlCommands {
        IocSyncptReadCommand = 0xC0080014,
        IocSyncptIncrCommand = 0x40040015,
        IocSyncptWaitCommand = 0xC00C0016,
        IocModuleMutexCommand = 0x40080017,
        IocModuleRegRDWRCommand = 0xC008010E,
        IocSyncptWaitexCommand = 0xC0100019,
        IocSyncptReadMaxCommand = 0xC008001A,
        IocGetConfigCommand = 0xC183001B,
    };

    struct IocGetConfigParams {
        std::array<char, 0x41> domain_str;
        std::array<char, 0x41> param_str;
        std::array<char, 0x101> config_str;
    };

    u32 NvOsGetConfigU32(const std::vector<u8>& input, std::vector<u8>& output);
};

} // namespace Devices
} // namespace Nvidia
} // namespace Service
