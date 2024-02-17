// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/service/bcat/news/overwrite_event_holder.h"
#include "core/hle/service/cmif_serialization.h"

namespace Service::News {

IOverwriteEventHolder::IOverwriteEventHolder(Core::System& system_)
    : ServiceFramework{system_, "IOverwriteEventHolder"},
      service_context{system_, "IOverwriteEventHolder"}, overwrite_event{service_context} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, D<&IOverwriteEventHolder::Get>, "Get"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IOverwriteEventHolder::~IOverwriteEventHolder() = default;

Result IOverwriteEventHolder::Get(OutCopyHandle<Kernel::KReadableEvent> out_event) {
    LOG_INFO(Service_BCAT, "called");

    *out_event = overwrite_event.GetHandle();
    R_SUCCEED();
}

} // namespace Service::News
