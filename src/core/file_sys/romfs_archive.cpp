// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include <memory>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/file_sys/romfs_archive.h"

namespace FileSys {

std::string ROMFSArchive::GetName() const {
    return "RomFS";
}

ResultVal<std::unique_ptr<FileBackend>> ROMFSArchive::OpenFile(const Path& path,
                                                               const Mode& mode) const {
    return MakeResult<std::unique_ptr<FileBackend>>(
        std::make_unique<ROMFSFile>(romfs_file, data_offset, data_size));
}

ResultCode ROMFSArchive::DeleteFile(const Path& path) const {
    LOG_CRITICAL(Service_FS, "Attempted to delete a file from an ROMFS archive (%s).",
                 GetName().c_str());
    // TODO(bunnei): Use correct error code
    return ResultCode(-1);
}

ResultCode ROMFSArchive::RenameFile(const Path& src_path, const Path& dest_path) const {
    LOG_CRITICAL(Service_FS, "Attempted to rename a file within an ROMFS archive (%s).",
                 GetName().c_str());
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode ROMFSArchive::DeleteDirectory(const Path& path) const {
    LOG_CRITICAL(Service_FS, "Attempted to delete a directory from an ROMFS archive (%s).",
                 GetName().c_str());
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode ROMFSArchive::DeleteDirectoryRecursively(const Path& path) const {
    LOG_CRITICAL(Service_FS, "Attempted to delete a directory from an ROMFS archive (%s).",
                 GetName().c_str());
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode ROMFSArchive::CreateFile(const Path& path, u64 size) const {
    LOG_CRITICAL(Service_FS, "Attempted to create a file in an ROMFS archive (%s).",
                 GetName().c_str());
    // TODO(bunnei): Use correct error code
    return ResultCode(-1);
}

ResultCode ROMFSArchive::CreateDirectory(const Path& path) const {
    LOG_CRITICAL(Service_FS, "Attempted to create a directory in an ROMFS archive (%s).",
                 GetName().c_str());
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultCode ROMFSArchive::RenameDirectory(const Path& src_path, const Path& dest_path) const {
    LOG_CRITICAL(Service_FS, "Attempted to rename a file within an ROMFS archive (%s).",
                 GetName().c_str());
    // TODO(wwylele): Use correct error code
    return ResultCode(-1);
}

ResultVal<std::unique_ptr<DirectoryBackend>> ROMFSArchive::OpenDirectory(const Path& path) const {
    return MakeResult<std::unique_ptr<DirectoryBackend>>(std::make_unique<ROMFSDirectory>());
}

u64 ROMFSArchive::GetFreeBytes() const {
    LOG_WARNING(Service_FS, "Attempted to get the free space in an ROMFS archive");
    return 0;
}

ResultVal<size_t> ROMFSFile::Read(const u64 offset, const size_t length, u8* buffer) const {
    LOG_TRACE(Service_FS, "called offset=%llu, length=%zu", offset, length);
    romfs_file->Seek(data_offset + offset, SEEK_SET);
    size_t read_length = (size_t)std::min((u64)length, data_size - offset);

    return MakeResult<size_t>(romfs_file->ReadBytes(buffer, read_length));
}

ResultVal<size_t> ROMFSFile::Write(const u64 offset, const size_t length, const bool flush,
                                   const u8* buffer) const {
    LOG_ERROR(Service_FS, "Attempted to write to ROMFS file");
    // TODO(Subv): Find error code
    return MakeResult<size_t>(0);
}

u64 ROMFSFile::GetSize() const {
    return data_size;
}

bool ROMFSFile::SetSize(const u64 size) const {
    LOG_ERROR(Service_FS, "Attempted to set the size of an ROMFS file");
    return false;
}

} // namespace FileSys
