// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <memory>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/file_sys/romfs_factory.h"
#include "core/file_sys/romfs_filesystem.h"

namespace FileSys {

RomFS_Factory::RomFS_Factory(Loader::AppLoader& app_loader) {
    // Load the RomFS from the app
    if (Loader::ResultStatus::Success != app_loader.ReadRomFS(romfs_file, data_offset, data_size)) {
        LOG_ERROR(Service_FS, "Unable to read RomFS!");
    }
}

ResultVal<std::unique_ptr<FileSystemBackend>> RomFS_Factory::Open(const Path& path) {
    auto archive = std::make_unique<RomFS_FileSystem>(romfs_file, data_offset, data_size);
    return MakeResult<std::unique_ptr<FileSystemBackend>>(std::move(archive));
}

ResultCode RomFS_Factory::Format(const Path& path) {
    LOG_ERROR(Service_FS, "Unimplemented Format archive %s", GetName().c_str());
    // TODO(bunnei): Find the right error code for this
    return ResultCode(-1);
}

ResultVal<ArchiveFormatInfo> RomFS_Factory::GetFormatInfo(const Path& path) const {
    LOG_ERROR(Service_FS, "Unimplemented GetFormatInfo archive %s", GetName().c_str());
    // TODO(bunnei): Find the right error code for this
    return ResultCode(-1);
}

} // namespace FileSys
