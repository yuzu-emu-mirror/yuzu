// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include <vector>
#include "common/common_types.h"
#include "core/file_sys/filesystem.h"
#include "core/hle/result.h"
#include "core/loader/loader.h"

namespace FileSys {

/// File system interface to the RomFS archive
class RomFS_Factory final : public FileSystemFactory {
public:
    explicit RomFS_Factory(Loader::AppLoader& app_loader);

    std::string GetName() const override {
        return "ArchiveFactory_RomFS";
    }
    ResultVal<std::unique_ptr<FileSystemBackend>> Open(const Path& path) override;
    ResultCode Format(const Path& path) override;
    ResultVal<ArchiveFormatInfo> GetFormatInfo(const Path& path) const override;

private:
    std::shared_ptr<FileUtil::IOFile> romfs_file;
    u64 data_offset;
    u64 data_size;
};

} // namespace FileSys
