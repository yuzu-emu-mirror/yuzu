// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/pm/debug_monitor_service.h"
#include "core/hle/service/pm/pm_types.h"

namespace Service::PM {

DebugMonitorService::DebugMonitorService(Core::System& system_)
    : ServiceFramework{system_, "pm:dmnt"} {
    // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetJitDebugProcessIdList"},
            {1, nullptr, "StartProcess"},
            {2, &DebugMonitorService::GetProcessId, "GetProcessId"},
            {3, nullptr, "HookToCreateProcess"},
            {4, &DebugMonitorService::GetApplicationProcessId, "GetApplicationProcessId"},
            {5, nullptr, "HookToCreateApplicationProgress"},
            {6, nullptr, "ClearHook"},
            {65000, &DebugMonitorService::AtmosphereGetProcessInfo, "AtmosphereGetProcessInfo"},
            {65001, nullptr, "AtmosphereGetCurrentLimitInfo"},
        };
    // clang-format on

    RegisterHandlers(functions);
}

void DebugMonitorService::GetProcessId(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto program_id = rp.PopRaw<u64>();

    LOG_DEBUG(Service_PM, "called, program_id={:016X}", program_id);

    auto list = kernel.GetProcessList();
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

void DebugMonitorService::GetApplicationProcessId(HLERequestContext& ctx) {
    LOG_DEBUG(Service_PM, "called");
    auto list = kernel.GetProcessList();
    GetApplicationPidGeneric(ctx, list);
}

void DebugMonitorService::AtmosphereGetProcessInfo(HLERequestContext& ctx) {
    // https://github.com/Atmosphere-NX/Atmosphere/blob/master/stratosphere/pm/source/impl/pm_process_manager.cpp#L614
    // This implementation is incomplete; only a handle to the process is returned.
    IPC::RequestParser rp{ctx};
    const auto pid = rp.PopRaw<u64>();

    LOG_WARNING(Service_PM, "(Partial Implementation) called, pid={:016X}", pid);

    auto list = kernel.GetProcessList();
    auto process = SearchProcessList(list, [pid](auto& p) { return p->GetProcessId() == pid; });

    if (process.IsNull()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultProcessNotFound);
        return;
    }

    struct ProgramLocation {
        u64 program_id;
        u8 storage_id;
    };
    static_assert(sizeof(ProgramLocation) == 0x10, "ProgramLocation has an invalid size");

    struct OverrideStatus {
        u64 keys_held;
        u64 flags;
    };
    static_assert(sizeof(OverrideStatus) == 0x10, "OverrideStatus has an invalid size");

    OverrideStatus override_status{};
    ProgramLocation program_location{
        .program_id = process->GetProgramId(),
        .storage_id = 0,
    };

    IPC::ResponseBuilder rb{ctx, 10, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(*process);
    rb.PushRaw(program_location);
    rb.PushRaw(override_status);
}

} // namespace Service::PM
