// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include <memory>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/file_sys/disk_filesystem.h"

namespace FileSys {

std::string Disk_FileSystem::GetName() const {
    return "Disk";
}

ResultVal<std::unique_ptr<StorageBackend>> Disk_FileSystem::OpenFile(const std::string& path,
                                                                     Mode mode) const {
    ASSERT_MSG(mode == Mode::Read || mode == Mode::Write, "Other file modes are not supported");

    std::string full_path = base_directory + path;
    auto file = std::make_shared<FileUtil::IOFile>(full_path, mode == Mode::Read ? "rb" : "wb");

    if (!file->IsOpen()) {
        // TODO(Subv): Find out the correct error code.
        return ResultCode(-1);
    }

    return MakeResult<std::unique_ptr<StorageBackend>>(
        std::make_unique<Disk_Storage>(std::move(file)));
}

ResultCode Disk_FileSystem::DeleteFile(const Path& path) const {
    LOG_WARNING(Service_FS, "(STUBBED) called");
    // TODO(bunnei): Use correct error code
    return ResultCode(-1);
}

ResultCode Disk_FileSystem::RenameFile(const Path& src_path, const Path& dest_path) const {
    LOG_WARNING(Service_FS, "(STUBBED) called");
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode Disk_FileSystem::DeleteDirectory(const Path& path) const {
    LOG_WARNING(Service_FS, "(STUBBED) called");
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode Disk_FileSystem::DeleteDirectoryRecursively(const Path& path) const {
    LOG_WARNING(Service_FS, "(STUBBED) called");
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode Disk_FileSystem::CreateFile(const std::string& path, u64 size) const {
    LOG_WARNING(Service_FS, "(STUBBED) called");

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

    LOG_ERROR(Service_FS, "Too large file");
    // TODO(Subv): Find out the correct error code
    return ResultCode(-1);
}

ResultCode Disk_FileSystem::CreateDirectory(const Path& path) const {
    LOG_WARNING(Service_FS, "(STUBBED) called");
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode Disk_FileSystem::RenameDirectory(const Path& src_path, const Path& dest_path) const {
    LOG_WARNING(Service_FS, "(STUBBED) called");
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultVal<std::unique_ptr<DirectoryBackend>> Disk_FileSystem::OpenDirectory(
    const Path& path) const {
    return MakeResult<std::unique_ptr<DirectoryBackend>>(std::make_unique<Disk_Directory>());
}

u64 Disk_FileSystem::GetFreeSpaceSize() const {
    LOG_WARNING(Service_FS, "(STUBBED) called");
    return 0;
}

ResultVal<FileSys::EntryType> Disk_FileSystem::GetEntryType(const std::string& path) const {
    std::string full_path = base_directory + path;
    if (!FileUtil::Exists(full_path)) {
        // TODO(Subv): Find out what this actually means
        return ResultCode(ErrorModule::FS, 1);
    }

    // TODO(Subv): Find out the EntryType values
    UNIMPLEMENTED_MSG("Unimplemented GetEntryType");
}

ResultVal<size_t> Disk_Storage::Read(const u64 offset, const size_t length, u8* buffer) const {
    LOG_TRACE(Service_FS, "called offset=%llu, length=%zu", offset, length);
    file->Seek(offset, SEEK_SET);
    return MakeResult<size_t>(file->ReadBytes(buffer, length));
}

ResultVal<size_t> Disk_Storage::Write(const u64 offset, const size_t length, const bool flush,
                                      const u8* buffer) const {
    LOG_WARNING(Service_FS, "(STUBBED) called");
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
    LOG_WARNING(Service_FS, "(STUBBED) called");
    return false;
}

u32 Disk_Directory::Read(const u32 count, Entry* entries) {
    LOG_WARNING(Service_FS, "(STUBBED) called");
    return 0;
}

bool Disk_Directory::Close() const {
    LOG_WARNING(Service_FS, "(STUBBED) called");
    return true;
}

} // namespace FileSys
