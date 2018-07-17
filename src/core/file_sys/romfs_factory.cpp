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

RomFSFactory::RomFSFactory(Loader::AppLoader& app_loader) {
    // Load the RomFS from the app
    if (Loader::ResultStatus::Success != app_loader.ReadRomFS(romfs_file, data_offset, data_size)) {
        LOG_ERROR(Service_FS, "Unable to read RomFS!");
    }
}

ResultVal<std::unique_ptr<FileSystemBackend>> RomFSFactory::Open(u64 title_id) {
    // TODO(DarkLordZach): Use title id.
    auto archive = std::make_unique<RomFS_FileSystem>(romfs_file, data_offset, data_size);
    return MakeResult<std::unique_ptr<FileSystemBackend>>(std::move(archive));
}

} // namespace FileSys
