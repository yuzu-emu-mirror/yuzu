// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/cmif_serialization.h"
#include "core/hle/service/filesystem/fsp/fs_i_program_registry.h"

namespace Service::FileSystem {

IProgramRegistry::IProgramRegistry(Core::System& system_)
    : ServiceFramework{system_, "fsp:pr"}, registry{system_.GetProgramRegistry()} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, C<&IProgramRegistry::RegisterProgram>, "RegisterProgram"},
        {1, C<&IProgramRegistry::UnregisterProgram>, "UnregisterProgram"},
        {2, C<&IProgramRegistry::SetCurrentProcess>, "SetCurrentProcess"},
        {256, C<&IProgramRegistry::SetEnabledProgramVerification>, "SetEnabledProgramVerification"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IProgramRegistry::~IProgramRegistry() = default;

Result IProgramRegistry::RegisterProgram(u8 storage_id, u64 process_id, u64 program_id,
                                         const InBuffer<BufferAttr_HipcMapAlias> data,
                                         s64 data_size,
                                         const InBuffer<BufferAttr_HipcMapAlias> desc,
                                         s64 desc_size) {
    LOG_INFO(Service_FS,
             "called, process_id={}, program_id={}, storage_id={}, data_size={}, desc_size = {}",
             process_id, program_id, storage_id, data_size, desc_size);
    R_RETURN(registry.RegisterProgram(process_id, program_id, storage_id, data, data_size, desc,
                                      desc_size));
}

Result IProgramRegistry::UnregisterProgram(u64 process_id) {
    LOG_INFO(Service_FS, "called, process_id={}", process_id);
    R_RETURN(registry.UnregisterProgram(process_id));
}

Result IProgramRegistry::SetCurrentProcess(const ClientProcessId& client_pid) {
    LOG_INFO(Service_FS, "called, client_pid={}", client_pid.pid);
    R_RETURN(registry.SetCurrentProcess(client_pid));
}

Result IProgramRegistry::SetEnabledProgramVerification(bool enabled) {
    LOG_INFO(Service_FS, "called, enabled={}", enabled);
    R_RETURN(registry.SetEnabledProgramVerification(enabled));
}

} // namespace Service::FileSystem
