// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include "common/common_funcs.h"
#include "common/intrusive_list.h"
#include "core/file_sys/fssrv/impl/fssrv_program_info.h"
#include "core/file_sys/romfs_factory.h"
#include "core/file_sys/savedata_factory.h"

namespace Core {
class System;
}

namespace FileSys::FsSrv::Impl {

class ProgramRegistryManager {
    YUZU_NON_COPYABLE(ProgramRegistryManager);
    YUZU_NON_MOVEABLE(ProgramRegistryManager);

public:
    explicit ProgramRegistryManager(Core::System& system_, InitialProgramInfo& program_info);

    Result RegisterProgram(u64 process_id, u64 program_id, u8 storage_id, const void* data,
                           s64 data_size, const void* desc, s64 desc_size,
                           std::shared_ptr<FileSys::RomFSFactory> romfs_factory = nullptr,
                           std::shared_ptr<FileSys::SaveDataFactory> save_data_factory = nullptr);
    Result UnregisterProgram(u64 process_id);

    Result GetProgramInfo(std::shared_ptr<ProgramInfo>* out, u64 process_id);
    Result GetProgramInfoByProgramId(std::shared_ptr<ProgramInfo>* out, u64 program_id);

private:
    using ProgramInfoList = Common::IntrusiveListBaseTraits<ProgramInfoNode>::ListType;

    ProgramInfoList m_program_info_list{};
    InitialProgramInfo& m_initial_program_info;
    mutable std::mutex m_mutex{};
    Core::System& system;
};

} // namespace FileSys::FsSrv::Impl
