// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/file_sys/vfs.h"
#include "core/hle/result.h"

namespace Loader {
class AppLoader;
}

namespace FileSys {

/// File system interface to the RomFS archive
class RomFSFactory {
public:
    explicit RomFSFactory(Loader::AppLoader& app_loader);

    ResultVal<VirtualFile> Open(u64 title_id);

private:
    VirtualFile file;
};

} // namespace FileSys
