// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/cmif_serialization.h"
#include "core/hle/service/pm/debug_monitor_service.h"

namespace Service::PM {

DebugMonitorService::DebugMonitorService(Core::System& system_)
    : ServiceFramework{system_, "pm:dmnt"} {
    // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetExceptionProcessIdList"},
            {1, nullptr, "StartProcess"},
            {2, C<&DebugMonitorService::GetProcessId>, "GetProcessId"},
            {3, nullptr, "HookToCreateProcess"},
            {4, C<&DebugMonitorService::GetApplicationProcessId>, "GetApplicationProcessId"},
            {5, nullptr, "HookToCreateApplicationProgress"},
            {6, nullptr, "ClearHook"},
            {65000, C<&DebugMonitorService::AtmosphereGetProcessInfo>, "AtmosphereGetProcessInfo"},
            {65001, nullptr, "AtmosphereGetCurrentLimitInfo"},
        };
    // clang-format on

    RegisterHandlers(functions);
}

Result DebugMonitorService::GetProcessId(Out<ProcessId> out_process_id, u64 program_id) {
    LOG_DEBUG(Service_PM, "called, program_id={:016X}", program_id);

    auto list = kernel.GetProcessList();
    auto process =
        SearchProcessList(list, [program_id](auto& p) { return p->GetProgramId() == program_id; });

    R_UNLESS(!process.IsNull(), ResultProcessNotFound);

    *out_process_id = ProcessId(process->GetProcessId());

    R_SUCCEED();
}

Result DebugMonitorService::GetApplicationProcessId(Out<ProcessId> out_process_id) {
    LOG_DEBUG(Service_PM, "called");
    auto list = kernel.GetProcessList();
    R_RETURN(GetApplicationPidGeneric(out_process_id, list));
}

Result DebugMonitorService::AtmosphereGetProcessInfo(
    OutCopyHandle<Kernel::KProcess> out_process_handle, Out<ProgramLocation> out_location,
    Out<OverrideStatus> out_status, ProcessId process_id) {
    // https://github.com/Atmosphere-NX/Atmosphere/blob/master/stratosphere/pm/source/impl/pm_process_manager.cpp#L614
    // This implementation is incomplete; only a handle to the process is returned.
    const auto pid = process_id.pid;

    LOG_WARNING(Service_PM, "(Partial Implementation) called, pid={:016X}", pid);

    auto list = kernel.GetProcessList();
    auto process = SearchProcessList(list, [pid](auto& p) { return p->GetProcessId() == pid; });
    R_UNLESS(!process.IsNull(), ResultProcessNotFound);

    OverrideStatus override_status{};
    ProgramLocation program_location{
        .program_id = process->GetProgramId(),
        .storage_id = 0,
    };

    *out_process_handle = process.GetPointerUnsafe();
    *out_location = program_location;
    *out_status = override_status;

    R_SUCCEED();
}

} // namespace Service::PM
