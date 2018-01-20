// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

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
    AppLoader_NRO(FileUtil::IOFile&& file, std::string filepath)
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
    bool LoadNro(const std::string& path, VAddr load_base);

    std::string filepath;
};

} // namespace Loader
