// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "common/common_types.h"
#include "core/file_sys/romfs_factory.h"
#include "core/file_sys/savedata_factory.h"
#include "core/file_sys/sdmc_factory.h"
#include "core/hle/result.h"

namespace FileSys {
class FileSystemBackend;
} // namespace FileSys

namespace Service {

namespace SM {
class ServiceManager;
} // namespace SM

namespace FileSystem {

ResultCode RegisterRomFS(std::unique_ptr<FileSys::RomFSFactory>&& factory);
ResultCode RegisterSaveData(std::unique_ptr<FileSys::SaveDataFactory>&& factory);
ResultCode RegisterSDMC(std::unique_ptr<FileSys::SDMCFactory>&& factory);

// TODO(DarkLordZach): BIS Filesystem
// ResultCode RegisterBIS(std::unique_ptr<FileSys::BISFactory>&& factory);

ResultVal<std::unique_ptr<FileSys::FileSystemBackend>> OpenRomFS(u64 title_id);
ResultVal<std::unique_ptr<FileSys::FileSystemBackend>> OpenSaveData(
    FileSys::SaveDataSpaceId space, FileSys::SaveDataDescriptor save_struct);
ResultVal<std::unique_ptr<FileSys::FileSystemBackend>> OpenSDMC();

// TODO(DarkLordZach): BIS Filesystem
// ResultVal<std::unique_ptr<FileSys::FileSystemBackend>> OpenBIS();

/// Registers all Filesystem services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace FileSystem
} // namespace Service
