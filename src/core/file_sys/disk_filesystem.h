// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include "common/common_types.h"
#include "common/file_util.h"
#include "core/file_sys/directory.h"
#include "core/file_sys/filesystem.h"
#include "core/file_sys/storage.h"
#include "core/hle/result.h"

namespace FileSys {

class Disk_FileSystem : public FileSystemBackend {
public:
    explicit Disk_FileSystem(std::string base_directory)
        : base_directory(std::move(base_directory)) {}

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
    std::string base_directory;
};

class Disk_Storage : public StorageBackend {
public:
    Disk_Storage(std::shared_ptr<FileUtil::IOFile> file) : file(std::move(file)) {}

    ResultVal<size_t> Read(u64 offset, size_t length, u8* buffer) const override;
    ResultVal<size_t> Write(u64 offset, size_t length, bool flush, const u8* buffer) const override;
    u64 GetSize() const override;
    bool SetSize(u64 size) const override;
    bool Close() const override {
        return false;
    }
    void Flush() const override {}

private:
    std::shared_ptr<FileUtil::IOFile> file;
};

class Disk_Directory : public DirectoryBackend {
public:
    Disk_Directory(const std::string& path);

    ~Disk_Directory() override {
        Close();
    }

    u64 Read(const u64 count, Entry* entries) override;
    u64 GetEntryCount() const override;

    bool Close() const override {
        return true;
    }

protected:
    u32 total_entries_in_directory;
    FileUtil::FSTEntry directory;

    // We need to remember the last entry we returned, so a subsequent call to Read will continue
    // from the next one. This iterator will always point to the next unread entry.
    std::vector<FileUtil::FSTEntry>::iterator children_iterator;
};

} // namespace FileSys
