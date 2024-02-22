// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/file_sys/fssrv/impl/fssrv_access_control.h"
#include "core/file_sys/romfs_factory.h"

namespace Core {
class System;
}

namespace FileSys::FsSrv::Impl {

constexpr u64 InvalidProcessId = 0xffffffffffffffffULL;

class ProgramInfo {
private:
    u64 m_process_id;
    u64 m_program_id;
    FileSys::StorageId m_storage_id;
    AccessControl m_access_control;

public:
    ProgramInfo(u64 process_id, u64 program_id, u8 storage_id, const void* data, s64 data_size,
                const void* desc, s64 desc_size)
        : m_process_id(process_id), m_access_control(data, data_size, desc, desc_size) {
        m_program_id = program_id;
        m_storage_id = static_cast<FileSys::StorageId>(storage_id);
    }

    bool Contains(u64 process_id) const {
        return m_process_id == process_id;
    }
    u64 GetProcessId() const {
        return m_process_id;
    }
    u64 GetProgramId() const {
        return m_program_id;
    }
    FileSys::StorageId GetStorageId() const {
        return m_storage_id;
    }
    AccessControl& GetAccessControl() {
        return m_access_control;
    }

    static std::shared_ptr<ProgramInfo> GetProgramInfoForInitialProcess();

private:
    const u64 InvalidProgramId = {};
    ProgramInfo(const void* data, s64 data_size, const void* desc, s64 desc_size)
        : m_process_id(InvalidProcessId), m_program_id(InvalidProgramId),
          m_storage_id(static_cast<FileSys::StorageId>(0)),
          m_access_control(data, data_size, desc, desc_size, std::numeric_limits<u64>::max()) {}
};

struct ProgramInfoNode : public Common::IntrusiveListBaseNode<ProgramInfoNode> {
    std::shared_ptr<ProgramInfo> program_info;
};

bool IsInitialProgram(Core::System& system, u64 process_id);
bool IsCurrentProcess(Core::System& system, u64 process_id);

} // namespace FileSys::FsSrv::Impl
