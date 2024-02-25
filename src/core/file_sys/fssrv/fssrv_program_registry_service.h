// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>
#include "core/file_sys/fs_program_index_map_info.h"
#include "core/file_sys/fssrv/impl/fssrv_program_index_map_info_manager.h"
#include "core/file_sys/fssrv/impl/fssrv_program_registry_manager.h"
#include "core/hle/result.h"

namespace Core {
class System;
}

namespace FileSys::FsSrv {

namespace Impl {
class ProgramInfo;
} // namespace Impl

class ProgramRegistryServiceImpl {
public:
    struct Configuration {};

    ProgramRegistryServiceImpl(Core::System& system_, const Configuration& cfg);

    Result RegisterProgramInfo(u64 process_id, u64 program_id, u8 storage_id, const void* data,
                               s64 data_size, const void* desc, s64 desc_size);
    Result UnregisterProgramInfo(u64 process_id);

    Result ResetProgramIndexMapInfo(const ProgramIndexMapInfo* infos, int count);

    Result GetProgramInfo(std::shared_ptr<Impl::ProgramInfo>* out, u64 process_id);
    Result GetProgramInfoByProgramId(std::shared_ptr<Impl::ProgramInfo>* out, u64 program_id);

    size_t GetProgramIndexMapInfoCount();
    std::optional<ProgramIndexMapInfo> GetProgramIndexMapInfo(const u64& program_id);

    u64 GetProgramIdByIndex(const u64& program_id, u8 index);

private:
    Configuration m_config;
    std::unique_ptr<Impl::ProgramRegistryManager> m_registry_manager;
    std::unique_ptr<Impl::ProgramIndexMapInfoManager> m_index_map_info_manager;
};

} // namespace FileSys::FsSrv
