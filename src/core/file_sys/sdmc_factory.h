// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include "common/common_types.h"
#include "core/file_sys/filesystem.h"
#include "core/hle/result.h"

namespace FileSys {

/// File system interface to the SDCard archive
class SDMC_Factory final : public FileSystemFactory {
public:
    explicit SDMC_Factory(std::string sd_directory);

    std::string GetName() const override {
        return "SDMC_Factory";
    }
    ResultVal<std::unique_ptr<FileSystemBackend>> Open(const Path& path) override;
    ResultCode Format(const Path& path) override;
    ResultVal<ArchiveFormatInfo> GetFormatInfo(const Path& path) const override;

private:
    std::string sd_directory;
};

} // namespace FileSys
