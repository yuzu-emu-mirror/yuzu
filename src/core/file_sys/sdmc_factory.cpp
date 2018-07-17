// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/file_sys/disk_filesystem.h"
#include "core/file_sys/sdmc_factory.h"

namespace FileSys {

SDMCFactory::SDMCFactory(std::string sd_directory) : sd_directory(std::move(sd_directory)) {}

ResultVal<std::unique_ptr<FileSystemBackend>> SDMCFactory::Open() {
    // Create the SD Card directory if it doesn't already exist.
    if (!FileUtil::IsDirectory(sd_directory)) {
        FileUtil::CreateFullPath(sd_directory);
    }

    auto archive = std::make_unique<Disk_FileSystem>(sd_directory);
    return MakeResult<std::unique_ptr<FileSystemBackend>>(std::move(archive));
}

} // namespace FileSys
