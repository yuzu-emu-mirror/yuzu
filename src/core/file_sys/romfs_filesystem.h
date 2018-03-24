// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include "common/common_types.h"
#include "common/file_util.h"
#include "core/file_sys/directory.h"
#include "core/file_sys/filesystem.h"
#include "core/file_sys/storage.h"
#include "core/hle/result.h"

namespace FileSys {

/**
 * Helper which implements an interface to deal with Switch .istorage ROMFS images used in some
 * archives This should be subclassed by concrete archive types, which will provide the input data
 * (load the raw ROMFS archive) and override any required methods
 */
class RomFS_FileSystem : public FileSystemBackend {
public:
    RomFS_FileSystem(std::shared_ptr<FileUtil::IOFile> file, u64 offset, u64 size)
        : romfs_file(file), data_offset(offset), data_size(size) {}

    std::string GetName() const override;

    ResultVal<std::unique_ptr<StorageBackend>> OpenFile(const std::string& path,
                                                        Mode mode) const override;
    ResultCode DeleteFile(const Path& path) const override;
    ResultCode RenameFile(const Path& src_path, const Path& dest_path) const override;
    ResultCode DeleteDirectory(const Path& path) const override;
    ResultCode DeleteDirectoryRecursively(const Path& path) const override;
    ResultCode CreateFile(const std::string& path, u64 size) const override;
    ResultCode CreateDirectory(const std::string& path) const override;
    ResultCode RenameDirectory(const Path& src_path, const Path& dest_path) const override;
    ResultVal<std::unique_ptr<DirectoryBackend>> OpenDirectory(
        const std::string& path) const override;
    u64 GetFreeSpaceSize() const override;
    ResultVal<EntryType> GetEntryType(const std::string& path) const override;

protected:
    std::shared_ptr<FileUtil::IOFile> romfs_file;
    u64 data_offset;
    u64 data_size;
};

class RomFS_Storage : public StorageBackend {
public:
    RomFS_Storage(std::shared_ptr<FileUtil::IOFile> file, u64 offset, u64 size)
        : romfs_file(file), data_offset(offset), data_size(size) {}

    ResultVal<size_t> Read(u64 offset, size_t length, u8* buffer) const override;
    ResultVal<size_t> Write(u64 offset, size_t length, bool flush, const u8* buffer) const override;
    u64 GetSize() const override;
    bool SetSize(u64 size) const override;
    bool Close() const override {
        return false;
    }
    void Flush() const override {}

private:
    std::shared_ptr<FileUtil::IOFile> romfs_file;
    u64 data_offset;
    u64 data_size;
};

class ROMFSDirectory : public DirectoryBackend {
public:
    u64 Read(const u64 count, Entry* entries) override {
        return 0;
    }
    u64 GetEntryCount() const override {
        return 0;
    }
    bool Close() const override {
        return false;
    }
};

} // namespace FileSys
