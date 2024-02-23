// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include "common/typed_storage.h"
#include "core/core.h"
#include "core/file_sys/fssrv/impl/fssrv_program_info.h"
#include "core/hle/kernel/svc.h"

namespace FileSys::FsSrv::Impl {

namespace {

constinit std::aligned_storage<0x80>::type g_static_buffer_for_program_info_for_initial_process =
    {};

template <typename T>
class StaticAllocatorForProgramInfoForInitialProcess : public std::allocator<T> {
public:
    StaticAllocatorForProgramInfoForInitialProcess() {}

    template <typename U>
    StaticAllocatorForProgramInfoForInitialProcess(
        const StaticAllocatorForProgramInfoForInitialProcess<U>&) {}

    template <typename U>
    struct rebind {
        using other = StaticAllocatorForProgramInfoForInitialProcess<U>;
    };

    [[nodiscard]] T* allocate(::std::size_t n) {
        ASSERT(sizeof(T) * n <= sizeof(g_static_buffer_for_program_info_for_initial_process));
        return reinterpret_cast<T*>(
            std::addressof(g_static_buffer_for_program_info_for_initial_process));
    }

    void deallocate([[maybe_unused]] T* p, [[maybe_unused]] std::size_t n) {
        // No-op
    }
};

constexpr const u32 FileAccessControlForInitialProgram[0x1C / sizeof(u32)] = {
    0x00000001, 0x00000000, 0x80000000, 0x0000001C, 0x00000000, 0x0000001C, 0x00000000};
constexpr const u32 FileAccessControlDescForInitialProgram[0x2C / sizeof(u32)] = {
    0x00000001, 0x00000000, 0x80000000, 0x00000000, 0x00000000, 0xFFFFFFFF,
    0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF};

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

    static constinit Common::TypedStorage<std::shared_ptr<ProgramInfo>>
        s_fls_storage_for_s_initial_program_info{};
    static constinit bool s_fls_initialized_s_initial_program_info = false;
    static std::mutex s_fls_init_lock_s_initial_program_info{};
    if (!(s_fls_initialized_s_initial_program_info)) {
        std::scoped_lock sl_fls_for_s_initial_program_info{s_fls_init_lock_s_initial_program_info};
        if (!(s_fls_initialized_s_initial_program_info)) {
            new (Common::Impl::GetPointerForConstructAt(s_fls_storage_for_s_initial_program_info))
                std::shared_ptr<ProgramInfo>(std::allocate_shared<ProgramInfoHelper>(
                    StaticAllocatorForProgramInfoForInitialProcess<char>{},
                    FileAccessControlForInitialProgram, sizeof(FileAccessControlForInitialProgram),
                    FileAccessControlDescForInitialProgram,
                    sizeof(FileAccessControlDescForInitialProgram)));
            s_fls_initialized_s_initial_program_info = true;
        }
    }
    std::shared_ptr<ProgramInfo>& s_initial_program_info =
        Common::GetReference(s_fls_storage_for_s_initial_program_info);

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
