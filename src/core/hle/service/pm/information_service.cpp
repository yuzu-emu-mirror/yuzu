// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/pm/information_service.h"
#include "core/hle/service/pm/pm_types.h"

namespace Service::PM {

InformationService::InformationService(Core::System& system_)
    : ServiceFramework{system_, "pm:info"} {
    static const FunctionInfo functions[] = {
        {0, &InformationService::GetProgramId, "GetProgramId"},
        {65000, &InformationService::AtmosphereGetProcessId, "AtmosphereGetProcessId"},
        {65001, nullptr, "AtmosphereHasLaunchedProgram"},
        {65002, nullptr, "AtmosphereGetProcessInfo"},
    };
    RegisterHandlers(functions);
}

void InformationService::GetProgramId(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto process_id = rp.PopRaw<u64>();

    LOG_DEBUG(Service_PM, "called, process_id={:016X}", process_id);

    auto list = kernel.GetProcessList();
    auto process =
        SearchProcessList(list, [process_id](auto& p) { return p->GetProcessId() == process_id; });

    if (process.IsNull()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultProcessNotFound);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push(process->GetProgramId());
}

void InformationService::AtmosphereGetProcessId(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto program_id = rp.PopRaw<u64>();

    LOG_DEBUG(Service_PM, "called, program_id={:016X}", program_id);

    auto list = system.Kernel().GetProcessList();
    auto process =
        SearchProcessList(list, [program_id](auto& p) { return p->GetProgramId() == program_id; });

    if (process.IsNull()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultProcessNotFound);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);
    rb.Push(process->GetProcessId());
}

} // namespace Service::PM
