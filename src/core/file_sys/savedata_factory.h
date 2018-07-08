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

/// File system interface to the SaveData archive
class SaveData_Factory final : public FileSystemFactory {
public:
    explicit SaveData_Factory(std::string nand_directory);

    std::string GetName() const override {
        return "SaveData_Factory";
    }
    ResultVal<std::unique_ptr<FileSystemBackend>> Open(const Path& path) override;
    ResultCode Format(const Path& path) override;
    ResultVal<ArchiveFormatInfo> GetFormatInfo(const Path& path) const override;

private:
    std::string nand_directory;

    std::string GetFullPath() const;
};

} // namespace FileSys
