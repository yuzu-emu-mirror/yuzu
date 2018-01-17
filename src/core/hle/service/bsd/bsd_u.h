// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/service.h"

namespace Service {
namespace BSD {

class BSD_U final : public ServiceFramework<BSD_U> {
public:
    BSD_U();
    ~BSD_U() = default;

private:
    void RegisterClient(Kernel::HLERequestContext& ctx);
    void Socket(Kernel::HLERequestContext& ctx);
    void Connect(Kernel::HLERequestContext& ctx);

    /// Id to use for the next open file descriptor.
    u32 next_fd = 1;
};

} // namespace BSD
} // namespace Service
