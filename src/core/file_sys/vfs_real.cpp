// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/file_sys/vfs_real.h"

namespace FileSys {

RealVfsFile::RealVfsFile(const std::string& path, const char openmode[])
    : backing(path, openmode), path(path), mode(openmode) {}

bool RealVfsFile::IsReady() {
    return backing.IsOpen();
}

bool RealVfsFile::IsGood() {
    return backing.IsGood();
}

void RealVfsFile::ResetState() {
    backing.Clear();
}

std::string RealVfsFile::GetName() {
    std::string part_path, name, extention;
    Common::SplitPath(path, &part_path, &name, &extention);
    return name + "." + extention;
}

u64 RealVfsFile::GetSize() {
    return backing.GetSize();
}

bool RealVfsFile::Resize(u64 new_size) {
    return backing.Resize(new_size);
}

std::shared_ptr<VfsDirectory> RealVfsFile::GetContainingDirectory() {
    std::string part_path, name, extention;
    Common::SplitPath(path, &part_path, &name, &extention);
    return std::make_unique<RealVfsDirectory>(part_path);
}

bool RealVfsFile::IsWritable() {
    return mode.find('w') != std::string::npos;
}

bool RealVfsFile::IsReadable() {
    return mode.find('r') != std::string::npos;
}

std::vector<u8> RealVfsFile::ReadBytes(u64 offset, u64 length) {
    backing.Seek(offset, SEEK_SET);
    std::vector<u8> out(length);
    backing.ReadBytes(out.data(), length);
    return out;
}

u64 RealVfsFile::WriteBytes(const std::vector<u8>& data, u64 offset) {
    backing.Seek(offset, SEEK_SET);
    return backing.WriteBytes(data.data(), data.size());
}

bool RealVfsFile::Rename(const std::string& name) {
    auto out = FileUtil::Rename(GetName(), name);
    std::string part_path, o_name, extention;
    Common::SplitPath(path, &part_path, &o_name, &extention);
    backing = FileUtil::IOFile(part_path + name, mode.c_str());
    return out;
}

} // namespace FileSys
