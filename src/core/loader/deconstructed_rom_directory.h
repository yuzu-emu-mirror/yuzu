// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include "common/common_types.h"
#include "common/file_util.h"
#include "core/hle/kernel/kernel.h"
#include "core/loader/loader.h"

namespace Loader {

/**
 * This class loads a "deconstructed ROM directory", which are the typical format we see for Switch
 * game dumps. The path should be a "main" NSO, which must be in a directory that contains the other
 * standard ExeFS NSOs (e.g. rtld, sdk, etc.). It will automatically find and load these.
 * Furthermore, it will look for the first .istorage file (optionally) and use this for the RomFS.
 */
class AppLoader_DeconstructedRomDirectory final : public AppLoader {
public:
    AppLoader_DeconstructedRomDirectory(FileUtil::IOFile&& file, std::string filepath)
        : AppLoader(std::move(file)), filepath(std::move(filepath)) {}

    /**
     * Returns the type of the file
     * @param file FileUtil::IOFile open file
     * @param filepath Path of the file that we are opening.
     * @return FileType found, or FileType::Error if this loader doesn't know it
     */
    static FileType IdentifyType(FileUtil::IOFile& file, const std::string& filepath);

    FileType GetFileType() override {
        return IdentifyType(file, filepath);
    }

    ResultStatus Load(Kernel::SharedPtr<Kernel::Process>& process) override;

private:
    std::string filepath;
};

} // namespace Loader
