// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/shared_memory.h"
#include "core/hle/service/service.h"

namespace Service {
namespace NS {

class PL_U final : public ServiceFramework<PL_U> {
public:
    PL_U();
    ~PL_U() = default;

private:
    void GetLoadState(Kernel::HLERequestContext& ctx);
    void GetSize(Kernel::HLERequestContext& ctx);
    void GetSharedMemoryAddressOffset(Kernel::HLERequestContext& ctx);
    void GetSharedMemoryNativeHandle(Kernel::HLERequestContext& ctx);

    // Handle to shared memory region designated for a shared font
    Kernel::SharedPtr<Kernel::SharedMemory> shared_mem;
};

/// Registers all AOC services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace NS
} // namespace Service
