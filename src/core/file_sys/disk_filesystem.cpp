// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include <memory>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/file_sys/disk_filesystem.h"
#include "core/file_sys/errors.h"

namespace FileSys {

static std::string ModeFlagsToString(Mode mode) {
    std::string mode_str;
    u32 mode_flags = static_cast<u32>(mode);

    // Calculate the correct open mode for the file.
    if ((mode_flags & static_cast<u32>(Mode::Read)) &&
        (mode_flags & static_cast<u32>(Mode::Write))) {
        if (mode_flags & static_cast<u32>(Mode::Append))
            mode_str = "a+";
        else
            mode_str = "r+";
    } else {
        if (mode_flags & static_cast<u32>(Mode::Read))
            mode_str = "r";
        else if (mode_flags & static_cast<u32>(Mode::Append))
            mode_str = "a";
        else if (mode_flags & static_cast<u32>(Mode::Write))
            mode_str = "w";
    }

    mode_str += "b";

    return mode_str;
}

std::string Disk_FileSystem::GetName() const {
    return "Disk";
}

ResultVal<std::unique_ptr<StorageBackend>> Disk_FileSystem::OpenFile(const std::string& path,
                                                                     Mode mode) const {

    // Calculate the correct open mode for the file.
    std::string mode_str = ModeFlagsToString(mode);

    std::string full_path = base_directory + path;
    auto file = std::make_shared<FileUtil::IOFile>(full_path, mode_str.c_str());

    if (!file->IsOpen()) {
        return ERROR_PATH_NOT_FOUND;
    }

    return MakeResult<std::unique_ptr<StorageBackend>>(
        std::make_unique<Disk_Storage>(std::move(file)));
}

ResultCode Disk_FileSystem::DeleteFile(const std::string& path) const {
    if (!FileUtil::Exists(path)) {
        return ERROR_PATH_NOT_FOUND;
    }

    FileUtil::Delete(path);

    return RESULT_SUCCESS;
}

ResultCode Disk_FileSystem::RenameFile(const std::string& src_path,
                                       const std::string& dest_path) const {
    const std::string full_src_path = base_directory + src_path;
    const std::string full_dest_path = base_directory + dest_path;

    if (!FileUtil::Exists(full_src_path)) {
        return ERROR_PATH_NOT_FOUND;
    }
    // TODO(wwylele): Use correct error code
    return FileUtil::Rename(full_src_path, full_dest_path) ? RESULT_SUCCESS : ResultCode(-1);
}

ResultCode Disk_FileSystem::DeleteDirectory(const Path& path) const {
    NGLOG_WARNING(Service_FS, "(STUBBED) called");
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode Disk_FileSystem::DeleteDirectoryRecursively(const Path& path) const {
    NGLOG_WARNING(Service_FS, "(STUBBED) called");
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode Disk_FileSystem::CreateFile(const std::string& path, u64 size) const {
    NGLOG_WARNING(Service_FS, "(STUBBED) called");

    std::string full_path = base_directory + path;
    if (size == 0) {
        FileUtil::CreateEmptyFile(full_path);
        return RESULT_SUCCESS;
    }

    FileUtil::IOFile file(full_path, "wb");
    // Creates a sparse file (or a normal file on filesystems without the concept of sparse files)
    // We do this by seeking to the right size, then writing a single null byte.
    if (file.Seek(size - 1, SEEK_SET) && file.WriteBytes("", 1) == 1) {
        return RESULT_SUCCESS;
    }

    NGLOG_ERROR(Service_FS, "Too large file");
    // TODO(Subv): Find out the correct error code
    return ResultCode(-1);
}

ResultCode Disk_FileSystem::CreateDirectory(const std::string& path) const {
    // TODO(Subv): Perform path validation to prevent escaping the emulator sandbox.
    std::string full_path = base_directory + path;

    if (FileUtil::CreateDir(full_path)) {
        return RESULT_SUCCESS;
    }

    NGLOG_CRITICAL(Service_FS, "(unreachable) Unknown error creating {}", full_path);
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode Disk_FileSystem::RenameDirectory(const Path& src_path, const Path& dest_path) const {
    NGLOG_WARNING(Service_FS, "(STUBBED) called");
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultVal<std::unique_ptr<DirectoryBackend>> Disk_FileSystem::OpenDirectory(
    const std::string& path) const {

    std::string full_path = base_directory + path;

    if (!FileUtil::IsDirectory(full_path)) {
        // TODO(Subv): Find the correct error code for this.
        return ResultCode(-1);
    }

    auto directory = std::make_unique<Disk_Directory>(full_path);
    return MakeResult<std::unique_ptr<DirectoryBackend>>(std::move(directory));
}

u64 Disk_FileSystem::GetFreeSpaceSize() const {
    NGLOG_WARNING(Service_FS, "(STUBBED) called");
    return 0;
}

ResultVal<FileSys::EntryType> Disk_FileSystem::GetEntryType(const std::string& path) const {
    std::string full_path = base_directory + path;
    if (!FileUtil::Exists(full_path)) {
        return ERROR_PATH_NOT_FOUND;
    }

    if (FileUtil::IsDirectory(full_path))
        return MakeResult(EntryType::Directory);

    return MakeResult(EntryType::File);
}

ResultVal<size_t> Disk_Storage::Read(const u64 offset, const size_t length, u8* buffer) const {
    NGLOG_TRACE(Service_FS, "called offset={}, length={}", offset, length);
    file->Seek(offset, SEEK_SET);
    return MakeResult<size_t>(file->ReadBytes(buffer, length));
}

ResultVal<size_t> Disk_Storage::Write(const u64 offset, const size_t length, const bool flush,
                                      const u8* buffer) const {
    NGLOG_WARNING(Service_FS, "(STUBBED) called");
    file->Seek(offset, SEEK_SET);
    size_t written = file->WriteBytes(buffer, length);
    if (flush) {
        file->Flush();
    }
    return MakeResult<size_t>(written);
}

u64 Disk_Storage::GetSize() const {
    return file->GetSize();
}

bool Disk_Storage::SetSize(const u64 size) const {
    file->Resize(size);
    file->Flush();
    return true;
}

Disk_Directory::Disk_Directory(const std::string& path) {
    unsigned size = FileUtil::ScanDirectoryTree(path, directory);
    directory.size = size;
    directory.isDirectory = true;
    children_iterator = directory.children.begin();
}

u64 Disk_Directory::Read(const u64 count, Entry* entries) {
    u64 entries_read = 0;

    while (entries_read < count && children_iterator != directory.children.cend()) {
        const FileUtil::FSTEntry& file = *children_iterator;
        const std::string& filename = file.virtualName;
        Entry& entry = entries[entries_read];

        NGLOG_TRACE(Service_FS, "File {}: size={} dir={}", filename, file.size, file.isDirectory);

        // TODO(Link Mauve): use a proper conversion to UTF-16.
        for (size_t j = 0; j < FILENAME_LENGTH; ++j) {
            entry.filename[j] = filename[j];
            if (!filename[j])
                break;
        }

        if (file.isDirectory) {
            entry.file_size = 0;
            entry.type = EntryType::Directory;
        } else {
            entry.file_size = file.size;
            entry.type = EntryType::File;
        }

        ++entries_read;
        ++children_iterator;
    }
    return entries_read;
}

u64 Disk_Directory::GetEntryCount() const {
    // We convert the children iterator into a const_iterator to allow template argument deduction
    // in std::distance.
    std::vector<FileUtil::FSTEntry>::const_iterator current = children_iterator;
    return std::distance(current, directory.children.end());
}

} // namespace FileSys
