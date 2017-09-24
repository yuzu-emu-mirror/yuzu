// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include "common/common_types.h"
#include "common/file_util.h"
#include "core/loader/loader.h"

namespace Loader {

/// Loads an NSO file
class AppLoader_NSO final : public AppLoader {
public:
    AppLoader_NSO(FileUtil::IOFile&& file, std::string filename, std::string filepath)
        : AppLoader(std::move(file)), filename(std::move(filename)), filepath(std::move(filepath)) {}

    /**
     * Returns the type of the file
     * @param file FileUtil::IOFile open file
     * @return FileType found, or FileType::Error if this loader doesn't know it
     */
    static FileType IdentifyType(FileUtil::IOFile& file);

    FileType GetFileType() override {
        return IdentifyType(file);
    }

    ResultStatus Load() override;

private:
    std::string filename;
    std::string filepath;
};

} // namespace Loader
