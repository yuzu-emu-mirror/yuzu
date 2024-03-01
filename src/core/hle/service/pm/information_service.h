// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/cmif_types.h"
#include "core/hle/service/service.h"

namespace Service::PM {

class InformationService final : public ServiceFramework<InformationService> {
public:
    explicit InformationService(Core::System& system_);

private:
    Result GetProgramId(Out<u64> out_program_id, u64 process_id);
    Result AtmosphereGetProcessId(Out<ProcessId> out_process_id, u64 program_id);
};

} // namespace Service::PM
