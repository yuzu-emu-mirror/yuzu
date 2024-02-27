// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/cmif_serialization.h"
#include "core/hle/service/pm/information_service.h"
#include "core/hle/service/pm/pm_types.h"

namespace Service::PM {

InformationService::InformationService(Core::System& system_)
    : ServiceFramework{system_, "pm:info"} {
    static const FunctionInfo functions[] = {
        {0, C<&InformationService::GetProgramId>, "GetProgramId"},
        {65000, C<&InformationService::AtmosphereGetProcessId>, "AtmosphereGetProcessId"},
        {65001, nullptr, "AtmosphereHasLaunchedProgram"},
        {65002, nullptr, "AtmosphereGetProcessInfo"},
    };
    RegisterHandlers(functions);
}

Result InformationService::GetProgramId(Out<u64> out_program_id, u64 process_id) {
    LOG_DEBUG(Service_PM, "called, process_id={:016X}", process_id);

    auto list = kernel.GetProcessList();
    auto process =
        SearchProcessList(list, [process_id](auto& p) { return p->GetProcessId() == process_id; });

    R_UNLESS(!process.IsNull(), ResultProcessNotFound);

    *out_program_id = process->GetProgramId();

    R_SUCCEED();
}

Result InformationService::AtmosphereGetProcessId(Out<ProcessId> out_process_id, u64 program_id) {
    LOG_DEBUG(Service_PM, "called, program_id={:016X}", program_id);

    auto list = system.Kernel().GetProcessList();
    auto process =
        SearchProcessList(list, [program_id](auto& p) { return p->GetProgramId() == program_id; });

    R_UNLESS(!process.IsNull(), ResultProcessNotFound);

    *out_process_id = ProcessId(process->GetProcessId());

    R_SUCCEED();
}

} // namespace Service::PM
