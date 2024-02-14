// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/cmif_serialization.h"
#include "core/hle/service/vi/application_display_service.h"
#include "core/hle/service/vi/service_creator.h"
#include "core/hle/service/vi/system_root_service.h"
#include "core/hle/service/vi/vi.h"
#include "core/hle/service/vi/vi_types.h"

namespace Service::VI {

ISystemRootService::ISystemRootService(Core::System& system_,
                                       std::shared_ptr<Nvnflinger::IHOSBinderDriver> binder_service,
                                       std::shared_ptr<FbshareBufferManager> shared_buffer_manager)
    : ServiceFramework{system_, "vi:s"}, m_binder_service{std::move(binder_service)},
      m_shared_buffer_manager{std::move(shared_buffer_manager)} {
    static const FunctionInfo functions[] = {
        {1, C<&ISystemRootService::GetDisplayService>, "GetDisplayService"},
        {3, nullptr, "GetDisplayServiceWithProxyNameExchange"},
    };
    RegisterHandlers(functions);
}

ISystemRootService::~ISystemRootService() = default;

Result ISystemRootService::GetDisplayService(
    Out<SharedPointer<IApplicationDisplayService>> out_application_display_service, Policy policy) {
    LOG_DEBUG(Service_VI, "called");
    R_RETURN(GetApplicationDisplayService(out_application_display_service, system, m_binder_service,
                                          m_shared_buffer_manager, Permission::System, policy));
}

} // namespace Service::VI
