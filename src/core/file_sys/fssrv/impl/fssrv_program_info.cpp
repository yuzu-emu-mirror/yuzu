// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include "common/typed_storage.h"
#include "core/core.h"
#include "core/file_sys/fssrv/impl/fssrv_program_info.h"
#include "core/hle/kernel/svc.h"
#include "core/hle/kernel/svc_common.h"

namespace FileSys::FsSrv::Impl {

std::shared_ptr<ProgramInfo> InitialProgramInfo::GetProgramInfoForInitialProcess() {
    constexpr const std::array<u32, 0x1C / sizeof(u32)> FileAccessControlForInitialProgram = {
        0x00000001, 0x00000000, 0x80000000, 0x0000001C, 0x00000000, 0x0000001C, 0x00000000};
    constexpr const std::array<u32, 0x2C / sizeof(u32)> FileAccessControlDescForInitialProgram = {
        0x00000001, 0x00000000, 0x80000000, 0x00000000, 0x00000000, 0xFFFFFFFF,
        0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF};

    if (!initial_program_info) {
        initial_program_info = std::make_shared<ProgramInfo>(
            FileAccessControlForInitialProgram.data(), sizeof(FileAccessControlForInitialProgram),
            FileAccessControlDescForInitialProgram.data(),
            sizeof(FileAccessControlDescForInitialProgram));
    }

    return initial_program_info;
}

void InitialProgramInfo::InitializeInitialAndCurrentProcessId(Core::System& system) {
    using namespace Kernel;

    if (!initialized_ids) {
        // Get initial process id range
        ASSERT(Svc::GetSystemInfo(system, std::addressof(initial_process_id_min),
                                  Svc::SystemInfoType::InitialProcessIdRange, Svc::InvalidHandle,
                                  static_cast<u64>(Svc::InitialProcessIdRangeInfo::Minimum)) ==
               ResultSuccess);
        ASSERT(Svc::GetSystemInfo(system, std::addressof(initial_process_id_max),
                                  Svc::SystemInfoType::InitialProcessIdRange, Svc::InvalidHandle,
                                  static_cast<u64>(Svc::InitialProcessIdRangeInfo::Maximum)) ==
               ResultSuccess);

        ASSERT(0 < initial_process_id_min);
        ASSERT(initial_process_id_min <= initial_process_id_max);

        // Get current procss id
        ASSERT(Svc::GetProcessId(system, std::addressof(current_process_id),
                                 Svc::PseudoHandle::CurrentProcess) == ResultSuccess);

        // Set initialized
        initialized_ids = true;
    }
}

bool InitialProgramInfo::IsInitialProgram(Core::System& system, u64 process_id) {
    // Initialize/sanity check
    InitializeInitialAndCurrentProcessId(system);
    ASSERT(initial_process_id_min > 0);

    // Check process id in range
    return initial_process_id_min <= process_id && process_id <= initial_process_id_max;
}

bool InitialProgramInfo::IsCurrentProcess(Core::System& system, u64 process_id) {
    // Initialize
    InitializeInitialAndCurrentProcessId(system);

    return process_id == current_process_id;
}

} // namespace FileSys::FsSrv::Impl
