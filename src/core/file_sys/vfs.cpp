// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <numeric>
#include "core/file_sys/vfs.h"

namespace FileSys {

VfsFile::~VfsFile() = default;

std::string VfsFile::GetExtension() const {
    size_t index = GetName().find_last_of('.');
    if (index == std::string::npos)
        return "";
    else
        return GetName().substr(index + 1);
}

VfsDirectory::~VfsDirectory() = default;

boost::optional<u8> VfsFile::ReadByte(size_t offset) const {
    u8 out{};
    size_t size = Read(&out, 1, offset);
    if (size == 1)
        return out;

    return boost::none;
}

std::vector<u8> VfsFile::ReadBytes(size_t size, size_t offset) const {
    std::vector<u8> out(size);
    size_t read_size = Read(out.data(), size, offset);
    out.resize(read_size);
    return out;
}

std::vector<u8> VfsFile::ReadAllBytes() const {
    return ReadBytes(GetSize());
}

bool VfsFile::WriteByte(u8 data, size_t offset) {
    return 1 == Write(&data, 1, offset);
}

size_t VfsFile::WriteBytes(std::vector<u8> data, size_t offset) {
    return Write(data.data(), data.size(), offset);
}

std::shared_ptr<VfsFile> VfsDirectory::GetFileRelative(const filesystem::path& path) const {
    if (path.parent_path() == path.root_path())
        return GetFile(path.filename().string());

    auto parent = path.parent_path().string();
    parent.replace(path.root_path().string().begin(), path.root_path().string().end(), "");
    const auto index = parent.find_first_of('/');
    const auto first_dir = parent.substr(0, index), rest = parent.substr(index + 1);
    const auto sub = GetSubdirectory(first_dir);
    if (sub == nullptr)
        return nullptr;
    return sub->GetFileRelative(path.root_path().string() + rest);
}

std::shared_ptr<VfsFile> VfsDirectory::GetFileAbsolute(const filesystem::path& path) const {
    if (IsRoot())
        return GetFileRelative(path);

    return GetParentDirectory()->GetFileAbsolute(path);
}

std::shared_ptr<VfsDirectory> VfsDirectory::GetDirectoryRelative(
    const filesystem::path& path) const {
    if (path.parent_path() == path.root_path())
        return GetSubdirectory(path.filename().string());

    auto parent = path.parent_path().string();
    parent.replace(path.root_path().string().begin(), path.root_path().string().end(), "");
    const auto index = parent.find_first_of('/');
    const auto first_dir = parent.substr(0, index), rest = parent.substr(index + 1);
    const auto sub = GetSubdirectory(first_dir);
    if (sub == nullptr)
        return nullptr;
    return sub->GetDirectoryRelative(path.root_path().string() + rest);
}

std::shared_ptr<VfsDirectory> VfsDirectory::GetDirectoryAbsolute(
    const filesystem::path& path) const {
    if (IsRoot())
        return GetDirectoryRelative(path);

    return GetParentDirectory()->GetDirectoryAbsolute(path);
}

std::shared_ptr<VfsFile> VfsDirectory::GetFile(const std::string& name) const {
    const auto& files = GetFiles();
    const auto& iter = std::find_if(files.begin(), files.end(), [&name](const auto& file1) {
        return name == file1->GetName();
    });
    return iter == files.end() ? nullptr : *iter;
}

std::shared_ptr<VfsDirectory> VfsDirectory::GetSubdirectory(const std::string& name) const {
    const auto& subs = GetSubdirectories();
    const auto& iter = std::find_if(
        subs.begin(), subs.end(), [&name](const auto& file1) { return name == file1->GetName(); });
    return iter == subs.end() ? nullptr : std::move(*iter);
}

bool VfsDirectory::IsRoot() const {
    return GetParentDirectory() == nullptr;
}

size_t VfsDirectory::GetSize() const {
    const auto& files = GetFiles();
    const auto file_total =
        std::accumulate(files.begin(), files.end(), 0ull,
                        [](const auto& f1, const auto& f2) { return f1 + f2->GetSize(); });

    const auto& sub_dir = GetSubdirectories();
    const auto subdir_total =
        std::accumulate(sub_dir.begin(), sub_dir.end(), 0ull,
                        [](const auto& f1, const auto& f2) { return f1 + f2->GetSize(); });

    return file_total + subdir_total;
}

bool VfsDirectory::DeleteSubdirectoryRecursive(const std::string& name) {
    auto dir = GetSubdirectory(name);
    if (dir == nullptr)
        return false;

    bool success = true;
    for (const auto& file : dir->GetFiles())
        if (!DeleteFile(file->GetName()))
            success = false;

    for (const auto& sdir : dir->GetSubdirectories())
        if (!dir->DeleteSubdirectoryRecursive(sdir->GetName()))
            success = false;

    return success;
}

bool VfsDirectory::Copy(const std::string& src, const std::string& dest) {
    const auto f1 = CreateFile(src);
    auto f2 = CreateFile(dest);
    if (f1 == nullptr || f2 == nullptr)
        return false;

    if (!f2->Resize(f1->GetSize())) {
        DeleteFile(dest);
        return false;
    }

    return f2->WriteBytes(f1->ReadAllBytes()) == f1->GetSize();
}

bool ReadOnlyVfsDirectory::IsWritable() const {
    return false;
}

bool ReadOnlyVfsDirectory::IsReadable() const {
    return true;
}

std::shared_ptr<VfsDirectory> ReadOnlyVfsDirectory::CreateSubdirectory(const std::string& name) {
    return nullptr;
}

std::shared_ptr<VfsFile> ReadOnlyVfsDirectory::CreateFile(const std::string& name) {
    return nullptr;
}

bool ReadOnlyVfsDirectory::DeleteSubdirectory(const std::string& name) {
    return false;
}

bool ReadOnlyVfsDirectory::DeleteFile(const std::string& name) {
    return false;
}

bool ReadOnlyVfsDirectory::Rename(const std::string& name) {
    return false;
}
} // namespace FileSys
