// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/cmif_types.h"
#include "core/hle/service/pm/pm_types.h"
#include "core/hle/service/service.h"

namespace Service::PM {

class DebugMonitorService final : public ServiceFramework<DebugMonitorService> {
public:
    explicit DebugMonitorService(Core::System& system_);

private:
    Result GetProcessId(Out<ProcessId> out_process_id, u64 program_id);
    Result GetApplicationProcessId(Out<ProcessId> out_process_id);
    Result AtmosphereGetProcessInfo(OutCopyHandle<Kernel::KProcess> out_process_handle,
                                    Out<ProgramLocation> out_location,
                                    Out<OverrideStatus> out_status, ProcessId process_id);
};
} // namespace Service::PM
