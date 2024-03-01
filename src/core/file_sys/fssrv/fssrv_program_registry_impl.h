// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_funcs.h"
#include "core/file_sys/fssrv/impl/fssrv_program_info.h"
#include "core/file_sys/romfs_factory.h"
#include "core/file_sys/savedata_factory.h"
#include "core/hle/result.h"
#include "core/hle/service/cmif_types.h"

namespace Core {
class System;
}

namespace FileSys::FsSrv {

using namespace Service;

class ProgramRegistryServiceImpl;

class ProgramRegistryImpl {
    YUZU_NON_COPYABLE(ProgramRegistryImpl);
    YUZU_NON_MOVEABLE(ProgramRegistryImpl);

public:
    ProgramRegistryImpl(Core::System& system_);
    ~ProgramRegistryImpl();

    Result RegisterProgram(u64 process_id, u64 program_id, u8 storage_id,
                           const InBuffer<BufferAttr_HipcMapAlias> data, s64 data_size,
                           const InBuffer<BufferAttr_HipcMapAlias> desc, s64 desc_size);
    // This function is not accurate to the Switch, but it is needed to work around current
    // limitations in our fs implementation
    Result RegisterProgramForLoader(u64 process_id, u64 program_id,
                                    std::shared_ptr<FileSys::RomFSFactory> romfs_factory,
                                    std::shared_ptr<FileSys::SaveDataFactory> save_data_factory);
    Result UnregisterProgram(u64 process_id);
    Result GetProgramInfo(std::shared_ptr<Impl::ProgramInfo>* out, u64 process_id);
    Result SetCurrentProcess(const Service::ClientProcessId& client_pid);
    Result SetEnabledProgramVerification(bool enabled);

    void Reset();

private:
    u64 m_process_id;
    Core::System& system;

    Impl::InitialProgramInfo initial_program_info{};
    std::unique_ptr<ProgramRegistryServiceImpl> service_impl;
};

} // namespace FileSys::FsSrv
