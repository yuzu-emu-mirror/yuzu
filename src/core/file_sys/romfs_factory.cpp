// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/file_sys/romfs_factory.h"
#include "core/loader/loader.h"

namespace FileSys {

RomFSFactory::RomFSFactory(Loader::AppLoader& app_loader) {
    // Load the RomFS from the app
    if (Loader::ResultStatus::Success != app_loader.ReadRomFS(file)) {
        LOG_ERROR(Service_FS, "Unable to read RomFS!");
    }
}

ResultVal<VirtualFile> RomFSFactory::Open(u64 title_id) {
    // TODO(DarkLordZach): Use title id.
    return MakeResult<VirtualFile>(file);
}

} // namespace FileSys
