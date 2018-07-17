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
class SDMCFactory {
public:
    explicit SDMCFactory(std::string sd_directory);

    ResultVal<std::unique_ptr<FileSystemBackend>> Open();

private:
    std::string sd_directory;
};

} // namespace FileSys
