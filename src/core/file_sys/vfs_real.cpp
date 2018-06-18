// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/common_paths.h"
#include "common/logging/log.h"
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
    return std::make_shared<RealVfsDirectory>(part_path, mode.c_str());
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

RealVfsDirectory::RealVfsDirectory(const std::string& path, const char openmode[])
    : path(path[path.size() - 1] == DIR_SEP_CHR ? path.substr(0, path.size() - 1) : path) {
    FileUtil::FSTEntry entry;
    if (0 != FileUtil::ScanDirectoryTree(path, entry))
        NGLOG_CRITICAL(Service_FS, "Failure to initialize file.");
    for (FileUtil::FSTEntry child : entry.children) {
        if (child.isDirectory)
            subdirectories.emplace_back(
                std::make_shared<RealVfsDirectory>(child.physicalName, mode.c_str()));
        else
            files.emplace_back(std::make_shared<RealVfsFile>(child.physicalName, mode.c_str()));
    }
}

bool RealVfsDirectory::IsReady() {
    return FileUtil::IsDirectory(path);
}

bool RealVfsDirectory::IsGood() {
    return FileUtil::IsDirectory(path);
}

void RealVfsDirectory::ResetState() {}

std::vector<std::shared_ptr<VfsFile>> RealVfsDirectory::GetFiles() {
    return std::vector<std::shared_ptr<VfsFile>>(files);
}

std::vector<std::shared_ptr<VfsDirectory>> RealVfsDirectory::GetSubdirectories() {
    return std::vector<std::shared_ptr<VfsDirectory>>(subdirectories);
}

bool RealVfsDirectory::IsWritable() {
    return mode.find('w') != std::string::npos;
}

bool RealVfsDirectory::IsReadable() {
    return mode.find('r') != std::string::npos;
}

bool RealVfsDirectory::IsRoot() {
    return path.empty() || (path.size() == 2 && path[1] == ':');
}

std::string RealVfsDirectory::GetName() {
    size_t last = path.rfind(DIR_SEP_CHR);
    return path.substr(last + 1);
}

std::shared_ptr<VfsDirectory> RealVfsDirectory::GetParentDirectory() {
    size_t last = path.rfind(DIR_SEP_CHR);
    if (last == path.size() - 1)
        last = path.rfind(DIR_SEP_CHR, path.size() - 2);

    if (last == std::string::npos)
        return nullptr;

    return std::make_shared<RealVfsDirectory>(path.substr(0, last), mode.c_str());
}

std::shared_ptr<VfsDirectory> RealVfsDirectory::CreateSubdirectory(const std::string& name) {
    FileUtil::CreateDir(path + DIR_SEP + name);
    subdirectories.emplace_back(
        std::make_shared<RealVfsDirectory>(path + DIR_SEP + name, mode.c_str()));
    return subdirectories[subdirectories.size() - 1];
}

std::shared_ptr<VfsFile> RealVfsDirectory::CreateFile(const std::string& name) {
    FileUtil::CreateEmptyFile(path + DIR_SEP + name);
    files.emplace_back(std::make_shared<RealVfsFile>(path + DIR_SEP + name, mode.c_str()));
    return files[files.size() - 1];
}

bool RealVfsDirectory::DeleteSubdirectory(const std::string& name) {
    return FileUtil::DeleteDirRecursively(path + DIR_SEP + name);
}

bool RealVfsDirectory::DeleteFile(const std::string& name) {
    return FileUtil::Delete(path + DIR_SEP + name);
}

bool RealVfsDirectory::Rename(const std::string& name) {
    std::string part, o_name, extention;
    Common::SplitPath(name, &part, &o_name, &extention);
    return FileUtil::Rename(path, part + DIR_SEP + name);
}

} // namespace FileSys
