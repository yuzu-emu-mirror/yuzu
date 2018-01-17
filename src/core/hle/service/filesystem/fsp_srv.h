// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service {
namespace FileSystem {

class FSP_SRV final : public ServiceFramework<FSP_SRV> {
public:
    FSP_SRV();
    ~FSP_SRV() = default;

private:
    void Initalize(Kernel::HLERequestContext& ctx);
    void GetGlobalAccessLogMode(Kernel::HLERequestContext& ctx);
    void OpenDataStorageByCurrentProcess(Kernel::HLERequestContext& ctx);
    void OpenRomStorage(Kernel::HLERequestContext& ctx);
};

} // namespace Filesystem
} // namespace Service
