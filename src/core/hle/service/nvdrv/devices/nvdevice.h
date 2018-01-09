// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "common/common_types.h"

namespace Service {
namespace NVDRV {
namespace Devices {

/// Represents an abstract nvidia device node. It is to be subclassed by concrete device nodes to
/// implement the ioctl interface.
class nvdevice {
public:
    nvdevice() = default;
    virtual ~nvdevice() = default;

    /**
     * Handles an ioctl request.
     * @param command The ioctl command id.
     * @param input A buffer containing the input data for the ioctl.
     * @param output A buffer where the output data will be written to.
     * @returns The result code of the ioctl.
     */
    virtual u32 ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) = 0;
};

} // namespace Devices
} // namespace NVDRV
} // namespace Service
