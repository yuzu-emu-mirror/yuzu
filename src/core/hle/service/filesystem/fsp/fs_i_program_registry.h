// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/file_sys/fssrv/fssrv_program_registry_impl.h"
#include "core/hle/result.h"
#include "core/hle/service/cmif_types.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::FileSystem {

class IProgramRegistry final : public ServiceFramework<IProgramRegistry> {
public:
    explicit IProgramRegistry(Core::System& system_);
    ~IProgramRegistry() override;

    Result RegisterProgram(u8 storage_id, u64 process_id, u64 program_id,
                           const InBuffer<BufferAttr_HipcMapAlias> data, s64 data_size,
                           const InBuffer<BufferAttr_HipcMapAlias> desc, s64 desc_size);

    Result UnregisterProgram(u64 process_id);

    Result SetCurrentProcess(const ClientProcessId& client_pid);

    Result SetEnabledProgramVerification(bool enabled);

private:
    FileSys::FsSrv::ProgramRegistryImpl registry;
};

} // namespace Service::FileSystem
