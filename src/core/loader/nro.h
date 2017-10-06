// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <map>
#include <string>
#include "common/common_types.h"
#include "common/file_util.h"
#include "core/hle/kernel/kernel.h"
#include "core/loader/linker.h"
#include "core/loader/loader.h"

namespace Loader {

/// Loads an NRO file
class AppLoader_NRO final : public AppLoader, Linker {
public:
    AppLoader_NRO(FileUtil::IOFile&& file, std::string filename, std::string filepath)
        : AppLoader(std::move(file)), filename(std::move(filename)), filepath(std::move(filepath)) {
    }

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
    VAddr GetEntryPoint(VAddr load_base) const;
    bool LoadNro(const std::string& path, VAddr load_base);

    std::string filename;
    std::string filepath;
};

} // namespace Loader
