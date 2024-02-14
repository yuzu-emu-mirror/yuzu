// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/cmif_types.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Nvnflinger {
class IHOSBinderDriver;
} // namespace Service::Nvnflinger

namespace Service::VI {

class FbshareBufferManager;
class IApplicationDisplayService;
enum class Policy : u32;

class IManagerRootService final : public ServiceFramework<IManagerRootService> {
public:
    explicit IManagerRootService(Core::System& system_,
                                 std::shared_ptr<Nvnflinger::IHOSBinderDriver> binder_service,
                                 std::shared_ptr<FbshareBufferManager> shared_buffer_manager);
    ~IManagerRootService() override;

    Result GetDisplayService(
        Out<SharedPointer<IApplicationDisplayService>> out_application_display_service,
        Policy policy);

private:
    const std::shared_ptr<Nvnflinger::IHOSBinderDriver> m_binder_service;
    const std::shared_ptr<FbshareBufferManager> m_shared_buffer_manager;
};

} // namespace Service::VI
