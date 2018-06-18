// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <numeric>
#include "core/file_sys/vfs.h"

namespace FileSys {

VfsFile::operator bool() {
    return IsGood();
}

boost::optional<u8> VfsFile::ReadByte(u64 offset) {
    auto vec = ReadBytes(offset, 1);
    if (vec.empty())
        return boost::none;
    return vec[0];
}

std::vector<u8> VfsFile::ReadAllBytes() {
    return ReadBytes(0, GetSize());
}

u64 VfsFile::ReplaceBytes(const std::vector<u8>& data) {
    if (!Resize(data.size()))
        return 0;
    return WriteBytes(data, 0);
}

VfsDirectory::operator bool() {
    return IsGood();
}

std::shared_ptr<VfsFile> VfsDirectory::GetFile(const std::string& name) {
    auto files = GetFiles();
    auto iter = std::find_if(files.begin(), files.end(),
                             [&name](auto file1) { return name == file1.GetName(); });
    return iter == files.end() ? nullptr : std::move(*iter);
}

std::shared_ptr<VfsDirectory> VfsDirectory::GetSubdirectory(const std::string& name) {
    auto subs = GetSubdirectories();
    auto iter = std::find_if(subs.begin(), subs.end(),
                             [&name](auto file1) { return name == file1.GetName(); });
    return iter == subs.end() ? nullptr : std::move(*iter);
}

u64 VfsDirectory::GetSize() {
    auto files = GetFiles();
    auto file_total = std::accumulate(files.begin(), files.end(), 0);

    auto sub_dir = GetSubdirectories();
    auto subdir_total = std::accumulate(sub_dir.begin(), sub_dir.end(), 0);

    return file_total + subdir_total;
}

bool VfsDirectory::Copy(const std::string& src, const std::string& dest) {
    auto f1 = CreateFile(src), f2 = CreateFile(dest);
    if (f1 == nullptr || f2 == nullptr)
        return false;

    return f2->ReplaceBytes(f1->ReadAllBytes()) == f1->GetSize();
}

VfsFile::~VfsFile() {}

VfsDirectory::~VfsDirectory() {}

} // namespace FileSys
