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
    enum IoctlCommands {
        IocSetNVMAPfdCommand = 0x40044801,
        IocSetClientDataCommand = 0x40084714,
        IocGetClientDataCommand = 0x80084715,
        IocZCullBind = 0xc010480b,
        IocSetErrorNotifierCommand = 0xC018480C,
    };

    struct set_nvmap_fd {
        u32_le nvmap_fd;
    };

    struct client_data {
        u64_le data;
    };

    struct zcull_bind {
        u64 gpu_va;
        u32 mode; // 0=global, 1=no_ctxsw, 2=separate_buffer, 3=part_of_regular_buf
        u32 padding;
    };

    struct set_error_notifier {
        u64_le offset;
        u64_le size;
        u32_le mem; // nvmap object handle
        u32_le padding;
    };

    u32_le nvmap_fd{};
    u64_le user_data{};
    zcull_bind zcull_params{};

    u32 SetNVMAPfd(const std::vector<u8>& input, std::vector<u8>& output);
    u32 SetClientData(const std::vector<u8>& input, std::vector<u8>& output);
    u32 GetClientData(const std::vector<u8>& input, std::vector<u8>& output);
    u32 ZCullBind(const std::vector<u8>& input, std::vector<u8>& output);
    u32 SetErrorNotifier(const std::vector<u8>& input, std::vector<u8>& output);
};

} // namespace Devices
} // namespace Nvidia
} // namespace Service
