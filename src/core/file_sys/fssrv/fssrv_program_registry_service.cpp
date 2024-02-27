// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/file_sys/fssrv/fssrv_program_registry_service.h"
#include "core/file_sys/fssrv/impl/fssrv_program_info.h"

namespace FileSys::FsSrv {

ProgramRegistryServiceImpl::ProgramRegistryServiceImpl(Core::System& system_,
                                                       Impl::InitialProgramInfo& program_info,
                                                       const Configuration& cfg)
    : m_config(cfg),
      m_registry_manager(std::make_unique<Impl::ProgramRegistryManager>(system_, program_info)),
      m_index_map_info_manager(std::make_unique<Impl::ProgramIndexMapInfoManager>()) {}

Result ProgramRegistryServiceImpl::RegisterProgramInfo(u64 process_id, u64 program_id,
                                                       u8 storage_id, const void* data,
                                                       s64 data_size, const void* desc,
                                                       s64 desc_size) {
    R_RETURN(m_registry_manager->RegisterProgram(process_id, program_id, storage_id, data,
                                                 data_size, desc, desc_size));
}

Result ProgramRegistryServiceImpl::RegisterProgramInfoForLoader(
    u64 process_id, u64 program_id, std::shared_ptr<FileSys::RomFSFactory> romfs_factory,
    std::shared_ptr<FileSys::SaveDataFactory> save_data_factory) {
    R_RETURN(m_registry_manager->RegisterProgram(process_id, program_id, 0, nullptr, 0, nullptr, 0,
                                                 romfs_factory, save_data_factory));
}

Result ProgramRegistryServiceImpl::UnregisterProgramInfo(u64 process_id) {
    R_RETURN(m_registry_manager->UnregisterProgram(process_id));
}

Result ProgramRegistryServiceImpl::ResetProgramIndexMapInfo(const ProgramIndexMapInfo* infos,
                                                            int count) {
    R_RETURN(m_index_map_info_manager->Reset(infos, count));
}

Result ProgramRegistryServiceImpl::GetProgramInfo(std::shared_ptr<Impl::ProgramInfo>* out,
                                                  u64 process_id) {
    R_RETURN(m_registry_manager->GetProgramInfo(out, process_id));
}

Result ProgramRegistryServiceImpl::GetProgramInfoByProgramId(
    std::shared_ptr<Impl::ProgramInfo>* out, u64 program_id) {
    R_RETURN(m_registry_manager->GetProgramInfoByProgramId(out, program_id));
}

size_t ProgramRegistryServiceImpl::GetProgramIndexMapInfoCount() {
    return m_index_map_info_manager->GetProgramCount();
}

std::optional<ProgramIndexMapInfo> ProgramRegistryServiceImpl::GetProgramIndexMapInfo(
    const u64& program_id) {
    return m_index_map_info_manager->Get(program_id);
}

u64 ProgramRegistryServiceImpl::GetProgramIdByIndex(const u64& program_id, u8 index) {
    return m_index_map_info_manager->GetProgramId(program_id, index);
}

} // namespace FileSys::FsSrv
