// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_funcs.h"
#include "core/hle/result.h"
#include "core/hle/service/cmif_types.h"

namespace Core {
class System;
}

namespace FileSys::FsSrv {

using namespace Service;

class ProgramRegistryServiceImpl;

namespace Impl {
class ProgramInfo;
}

class ProgramRegistryImpl {
    YUZU_NON_COPYABLE(ProgramRegistryImpl);
    YUZU_NON_MOVEABLE(ProgramRegistryImpl);

public:
    ProgramRegistryImpl(Core::System& system_);
    ~ProgramRegistryImpl();

    static void Initialize(ProgramRegistryServiceImpl* service);

    Result RegisterProgram(u64 process_id, u64 program_id, u8 storage_id,
                           const InBuffer<BufferAttr_HipcMapAlias> data, s64 data_size,
                           const InBuffer<BufferAttr_HipcMapAlias> desc, s64 desc_size);
    Result UnregisterProgram(u64 process_id);
    Result SetCurrentProcess(const Service::ClientProcessId& client_pid);
    Result SetEnabledProgramVerification(bool enabled);

private:
    u64 m_process_id;
    Core::System& system;
};

} // namespace FileSys::FsSrv
