// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include "common/typed_storage.h"
#include "core/core.h"
#include "core/file_sys/fssrv/impl/fssrv_program_info.h"
#include "core/hle/kernel/svc.h"
#include "core/hle/kernel/svc_common.h"

namespace FileSys::FsSrv::Impl {

namespace {

constinit bool s_fls_initialized_s_initial_program_info = false;

constinit bool g_initialized = false;

constinit u64 g_initial_process_id_min = 0;
constinit u64 g_initial_process_id_max = 0;

constinit u64 g_current_process_id = 0;

void InitializeInitialAndCurrentProcessId(Core::System& system) {
    using namespace Kernel;

    if (!g_initialized) {
        // Get initial process id range
        ASSERT(Svc::GetSystemInfo(system, std::addressof(g_initial_process_id_min),
                                  Svc::SystemInfoType::InitialProcessIdRange, Svc::InvalidHandle,
                                  static_cast<u64>(Svc::InitialProcessIdRangeInfo::Minimum)) ==
               ResultSuccess);
        ASSERT(Svc::GetSystemInfo(system, std::addressof(g_initial_process_id_max),
                                  Svc::SystemInfoType::InitialProcessIdRange, Svc::InvalidHandle,
                                  static_cast<u64>(Svc::InitialProcessIdRangeInfo::Maximum)) ==
               ResultSuccess);

        ASSERT(0 < g_initial_process_id_min);
        ASSERT(g_initial_process_id_min <= g_initial_process_id_max);

        // Get current procss id
        ASSERT(Svc::GetProcessId(system, std::addressof(g_current_process_id),
                                 Svc::PseudoHandle::CurrentProcess) == ResultSuccess);

        // Set initialized
        g_initialized = true;
    }
}

} // namespace

std::shared_ptr<ProgramInfo> ProgramInfo::GetProgramInfoForInitialProcess() {
    class ProgramInfoHelper : public ProgramInfo {
    public:
        ProgramInfoHelper(const void* data, s64 data_size, const void* desc, s64 desc_size)
            : ProgramInfo(data, data_size, desc, desc_size) {}
    };

    constexpr const std::array<u32, 0x1C / sizeof(u32)> FileAccessControlForInitialProgram = {
        0x00000001, 0x00000000, 0x80000000, 0x0000001C, 0x00000000, 0x0000001C, 0x00000000};
    constexpr const std::array<u32, 0x2C / sizeof(u32)> FileAccessControlDescForInitialProgram = {
        0x00000001, 0x00000000, 0x80000000, 0x00000000, 0x00000000, 0xFFFFFFFF,
        0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF};

    // if (!s_fls_initialized_s_initial_program_info) {
    std::shared_ptr<ProgramInfo> s_initial_program_info = std::make_shared<ProgramInfoHelper>(
        FileAccessControlForInitialProgram.data(), sizeof(FileAccessControlForInitialProgram),
        FileAccessControlDescForInitialProgram.data(),
        sizeof(FileAccessControlDescForInitialProgram));
    //}

    return s_initial_program_info;
}

bool IsInitialProgram(Core::System& system, u64 process_id) {
    // Initialize/sanity check
    InitializeInitialAndCurrentProcessId(system);
    ASSERT(g_initial_process_id_min > 0);

    // Check process id in range
    return g_initial_process_id_min <= process_id && process_id <= g_initial_process_id_max;
}

bool IsCurrentProcess(Core::System& system, u64 process_id) {
    // Initialize
    InitializeInitialAndCurrentProcessId(system);

    return process_id == g_current_process_id;
}

} // namespace FileSys::FsSrv::Impl
