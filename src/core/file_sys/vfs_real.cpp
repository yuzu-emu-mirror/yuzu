// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/file_sys/vfs_real.h"

namespace FileSys {

static const char* PermissionsToCharArray(filesystem::perms perms) {
    std::string out;
    if ((perms & filesystem::perms::owner_read) != filesystem::perms::none)
        out += "r";
    if ((perms & filesystem::perms::owner_write) != filesystem::perms::none)
        out += "w";
    return out.c_str();
}

RealVfsFile::RealVfsFile(const filesystem::path& path_, filesystem::perms perms_)
    : backing(path_.string(), PermissionsToCharArray(perms_)), path(path_), perms(perms_) {}

std::string RealVfsFile::GetName() const {
    return path.filename().string();
}

size_t RealVfsFile::GetSize() const {
    return backing.GetSize();
}

bool RealVfsFile::Resize(size_t new_size) {
    return backing.Resize(new_size);
}

std::shared_ptr<VfsDirectory> RealVfsFile::GetContainingDirectory() const {
    return std::make_shared<RealVfsDirectory>(path.parent_path(), perms);
}

bool RealVfsFile::IsWritable() const {
    return (perms & filesystem::perms::owner_write) != filesystem::perms::none;
}

bool RealVfsFile::IsReadable() const {
    return (perms & filesystem::perms::owner_read) != filesystem::perms::none;
}

size_t RealVfsFile::Read(u8* data, size_t length, size_t offset) const {
    if (!backing.Seek(offset, SEEK_SET))
        return 0;
    return backing.ReadBytes(data, length);
}

size_t RealVfsFile::Write(const u8* data, size_t length, size_t offset) {
    if (!backing.Seek(offset, SEEK_SET))
        return 0;
    return backing.WriteBytes(data, length);
}

bool RealVfsFile::Rename(const std::string& name) {
    const auto out = FileUtil::Rename(GetName(), name);
    path = path.parent_path() / name;
    backing = FileUtil::IOFile(path.string(), PermissionsToCharArray(perms));
    return out;
}

RealVfsDirectory::RealVfsDirectory(const filesystem::path& path_, filesystem::perms perms_)
    : path(path_), perms(perms_) {
    for (const auto& entry : filesystem::directory_iterator(path)) {
        if (filesystem::is_directory(entry.path()))
            subdirectories.emplace_back(std::make_shared<RealVfsDirectory>(entry.path(), perms));
        else if (filesystem::is_regular_file(entry.path()))
            files.emplace_back(std::make_shared<RealVfsFile>(entry.path(), perms));
    }
}

std::vector<std::shared_ptr<VfsFile>> RealVfsDirectory::GetFiles() const {
    return std::vector<std::shared_ptr<VfsFile>>(files);
}

std::vector<std::shared_ptr<VfsDirectory>> RealVfsDirectory::GetSubdirectories() const {
    return std::vector<std::shared_ptr<VfsDirectory>>(subdirectories);
}

bool RealVfsDirectory::IsWritable() const {
    return (perms & filesystem::perms::owner_write) != filesystem::perms::none;
}

bool RealVfsDirectory::IsReadable() const {
    return (perms & filesystem::perms::owner_read) != filesystem::perms::none;
}

std::string RealVfsDirectory::GetName() const {
    return path.filename().string();
}

std::shared_ptr<VfsDirectory> RealVfsDirectory::GetParentDirectory() const {
    if (path.parent_path() == path.root_path())
        return nullptr;

    return std::make_shared<RealVfsDirectory>(path.parent_path(), perms);
}

std::shared_ptr<VfsDirectory> RealVfsDirectory::CreateSubdirectory(const std::string& name) {
    if (!FileUtil::CreateDir((path / name).string()))
        return nullptr;
    subdirectories.emplace_back(std::make_shared<RealVfsDirectory>(path / name, perms));
    return subdirectories.back();
}

std::shared_ptr<VfsFile> RealVfsDirectory::CreateFile(const std::string& name) {
    if (!FileUtil::CreateEmptyFile((path / name).string()))
        return nullptr;
    files.emplace_back(std::make_shared<RealVfsFile>(path / name, perms));
    return files.back();
}

bool RealVfsDirectory::DeleteSubdirectory(const std::string& name) {
    return FileUtil::DeleteDirRecursively((path / name).string());
}

bool RealVfsDirectory::DeleteFile(const std::string& name) {
    return FileUtil::Delete((path / name).string());
}

bool RealVfsDirectory::Rename(const std::string& name) {
    return FileUtil::Rename(path.string(), (path / name).string());
}

} // namespace FileSys
