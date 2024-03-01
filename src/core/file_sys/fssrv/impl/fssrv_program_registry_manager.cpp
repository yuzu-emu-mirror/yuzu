// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/file_sys/errors.h"
#include "core/file_sys/fssrv/impl/fssrv_program_registry_manager.h"

namespace FileSys::FsSrv::Impl {

ProgramRegistryManager::ProgramRegistryManager(Core::System& system_,
                                               InitialProgramInfo& program_info)
    : m_initial_program_info{program_info}, system{system_} {}

Result ProgramRegistryManager::RegisterProgram(
    u64 process_id, u64 program_id, u8 storage_id, const void* data, s64 data_size,
    const void* desc, s64 desc_size, std::shared_ptr<FileSys::RomFSFactory> romfs_factory,
    std::shared_ptr<FileSys::SaveDataFactory> save_data_factory) {
    // Allocate a new node
    std::unique_ptr<ProgramInfoNode> new_node(new ProgramInfoNode());
    R_UNLESS(new_node != nullptr, ResultAllocationMemoryFailedInProgramRegistryManagerA);

    // Create a new program info
    {
        // Allocate the new info
        std::shared_ptr<ProgramInfo> new_info =
            std::make_shared<ProgramInfo>(process_id, program_id, storage_id, data, data_size, desc,
                                          desc_size, romfs_factory, save_data_factory);
        R_UNLESS(new_info != nullptr, ResultAllocationMemoryFailedInProgramRegistryManagerA);

        // Set the info in the node
        new_node->program_info = std::move(new_info);
    }

    // Acquire exclusive access to the registry
    std::scoped_lock lk(m_mutex);

    // Check that the process isn't already in the registry
    for (const auto& node : m_program_info_list) {
        R_UNLESS(!node.program_info->Contains(process_id), ResultInvalidArgument);
    }

    // Add the node to the registry
    m_program_info_list.push_back(*new_node.release());

    R_SUCCEED();
}

Result ProgramRegistryManager::UnregisterProgram(u64 process_id) {
    // Acquire exclusive access to the registry
    std::scoped_lock lk(m_mutex);

    // Try to find and remove the process's node
    for (auto& node : m_program_info_list) {
        if (node.program_info->Contains(process_id)) {
            m_program_info_list.erase(m_program_info_list.iterator_to(node));
            delete std::addressof(node);
            R_SUCCEED();
        }
    }

    // We couldn't find/unregister the process's node
    R_THROW(ResultInvalidArgument);
}

Result ProgramRegistryManager::GetProgramInfo(std::shared_ptr<ProgramInfo>* out, u64 process_id) {
    // Acquire exclusive access to the registry
    std::scoped_lock lk(m_mutex);

    // Check if we're getting permissions for an initial program
    if (m_initial_program_info.IsInitialProgram(system, process_id)) {
        *out = (m_initial_program_info.GetProgramInfoForInitialProcess());
        R_SUCCEED();
    }

    // Find a matching node
    for (const auto& node : m_program_info_list) {
        if (node.program_info->Contains(process_id)) {
            *out = node.program_info;
            R_SUCCEED();
        }
    }

    // We didn't find the program info
    R_THROW(ResultProgramInfoNotFound);
}

Result ProgramRegistryManager::GetProgramInfoByProgramId(std::shared_ptr<ProgramInfo>* out,
                                                         u64 program_id) {
    // Acquire exclusive access to the registry
    std::scoped_lock lk(m_mutex);

    // Find a matching node
    for (const auto& node : m_program_info_list) {
        if (node.program_info->GetProgramId() == program_id) {
            *out = node.program_info;
            R_SUCCEED();
        }
    }

    // We didn't find the program info
    R_THROW(ResultProgramInfoNotFound);
}

} // namespace FileSys::FsSrv::Impl
