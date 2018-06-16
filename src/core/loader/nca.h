// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include "common/common_types.h"
#include "core/file_sys/partition_filesystem.h"
#include "core/file_sys/program_metadata.h"
#include "core/hle/kernel/kernel.h"
#include "core/loader/linker.h"
#include "core/loader/loader.h"

namespace Loader {

class Nca {
    FileSys::PartitionFilesystem GetPFS(u8 id);

    u8 GetExeFsPfsId();

    u64 GetExeFsFileOffset();
    u64 GetExeFsFileSize();

    u64 GetRomFSOffset();
    u64 GetRomFSSize();

private:
    std::vector<FileSys::PartitionFilesystem> pfs;
    std::string path;
};

/// Loads an NCA file
class AppLoader_NCA final : public AppLoader, Linker {
public:
    AppLoader_NCA(FileUtil::IOFile&& file, std::string filepath);

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
    FileSys::ProgramMetadata metadata;
};

} // namespace Loader
