// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/file_sys/errors.h"
#include "core/file_sys/fssrv/fssrv_program_registry_impl.h"
#include "core/file_sys/fssrv/fssrv_program_registry_service.h"

namespace FileSys::FsSrv {

// TODO: Move this to a common types file
constexpr u64 InvalidProcessIdProgramRegistry = 0xffffffffffffffffULL;

ProgramRegistryImpl::ProgramRegistryImpl(Core::System& system_)
    : m_process_id(InvalidProcessIdProgramRegistry), system{system_},
      service_impl{std::make_unique<ProgramRegistryServiceImpl>(
          system, initial_program_info, ProgramRegistryServiceImpl::Configuration{})} {}

ProgramRegistryImpl::~ProgramRegistryImpl() {}

Result ProgramRegistryImpl::RegisterProgram(u64 process_id, u64 program_id, u8 storage_id,
                                            const InBuffer<BufferAttr_HipcMapAlias> data,
                                            s64 data_size,
                                            const InBuffer<BufferAttr_HipcMapAlias> desc,
                                            s64 desc_size) {
    // Check that we're allowed to register
    R_UNLESS(initial_program_info.IsInitialProgram(system, m_process_id), ResultPermissionDenied);

    // Check buffer sizes
    R_UNLESS(data.size() >= static_cast<size_t>(data_size), ResultInvalidSize);
    R_UNLESS(desc.size() >= static_cast<size_t>(desc_size), ResultInvalidSize);

    // Register the program
    R_RETURN(service_impl->RegisterProgramInfo(process_id, program_id, storage_id, data.data(),
                                               data_size, desc.data(), desc_size));
}

Result ProgramRegistryImpl::RegisterProgramForLoader(
    u64 process_id, u64 program_id, std::shared_ptr<FileSys::RomFSFactory> romfs_factory,
    std::shared_ptr<FileSys::SaveDataFactory> save_data_factory) {
    R_RETURN(service_impl->RegisterProgramInfoForLoader(process_id, program_id, romfs_factory,
                                                        save_data_factory));
}

Result ProgramRegistryImpl::UnregisterProgram(u64 process_id) {
    // Check that we're allowed to register
    R_UNLESS(initial_program_info.IsInitialProgram(system, m_process_id), ResultPermissionDenied);

    // Unregister the program
    R_RETURN(service_impl->UnregisterProgramInfo(process_id));
}

Result ProgramRegistryImpl::GetProgramInfo(std::shared_ptr<Impl::ProgramInfo>* out,
                                           u64 process_id) {
    R_RETURN(service_impl->GetProgramInfo(out, process_id));
}

Result ProgramRegistryImpl::SetCurrentProcess(const Service::ClientProcessId& client_pid) {
    // Set our process id
    m_process_id = client_pid.pid;

    R_SUCCEED();
}

Result ProgramRegistryImpl::SetEnabledProgramVerification(bool enabled) {
    // TODO: How to deal with this backwards compat?
    ASSERT_MSG(false, "TODO: SetEnabledProgramVerification");
    R_THROW(ResultNotImplemented);
}

void ProgramRegistryImpl::Reset() {
    initial_program_info = {};
    service_impl = std::make_unique<ProgramRegistryServiceImpl>(
        system, initial_program_info, ProgramRegistryServiceImpl::Configuration{});
}

} // namespace FileSys::FsSrv
