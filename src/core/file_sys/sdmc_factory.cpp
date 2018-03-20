// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include <memory>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/file_sys/disk_filesystem.h"
#include "core/file_sys/sdmc_factory.h"

namespace FileSys {

SDMC_Factory::SDMC_Factory(std::string sd_directory) : sd_directory(std::move(sd_directory)) {}

ResultVal<std::unique_ptr<FileSystemBackend>> SDMC_Factory::Open(const Path& path) {
    // Create the SD Card directory if it doesn't already exist.
    if (!FileUtil::IsDirectory(sd_directory)) {
        FileUtil::CreateFullPath(sd_directory);
    }

    auto archive = std::make_unique<Disk_FileSystem>(sd_directory);
    return MakeResult<std::unique_ptr<FileSystemBackend>>(std::move(archive));
}

ResultCode SDMC_Factory::Format(const Path& path) {
    LOG_ERROR(Service_FS, "Unimplemented Format archive %s", GetName().c_str());
    // TODO(Subv): Find the right error code for this
    return ResultCode(-1);
}

ResultVal<ArchiveFormatInfo> SDMC_Factory::GetFormatInfo(const Path& path) const {
    LOG_ERROR(Service_FS, "Unimplemented GetFormatInfo archive %s", GetName().c_str());
    // TODO(bunnei): Find the right error code for this
    return ResultCode(-1);
}

} // namespace FileSys
