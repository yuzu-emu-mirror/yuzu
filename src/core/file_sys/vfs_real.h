// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once
#include "common/file_util.h"
#include "core\file_sys\vfs.h"

namespace FileSys {

struct RealVfsFile : public VfsFile {
    explicit RealVfsFile(const std::string& name, const char openmode[]);

    bool IsReady() override;
    bool IsGood() override;
    void ResetState() override;
    std::string GetName() override;
    u64 GetSize() override;
    bool Resize(u64 new_size) override;
    std::shared_ptr<VfsDirectory> GetContainingDirectory() override;
    bool IsWritable() override;
    bool IsReadable() override;
    std::vector<u8> ReadBytes(u64 offset, u64 length) override;
    u64 WriteBytes(const std::vector<u8>& data, u64 offset) override;
    bool Rename(const std::string& name) override;

private:
    FileUtil::IOFile backing;
    std::string path;
    std::string mode;
};

struct RealVfsDirectory : public VfsDirectory {
    explicit RealVfsDirectory(const std::string& path, const char openmode[]);

    bool IsReady() override;
    bool IsGood() override;
    void ResetState() override;
    std::vector<std::shared_ptr<VfsFile>> GetFiles() override;
    std::vector<std::shared_ptr<VfsDirectory>> GetSubdirectories() override;
    bool IsWritable() override;
    bool IsReadable() override;
    bool IsRoot() override;
    std::string GetName() override;
    std::shared_ptr<VfsDirectory> GetParentDirectory() override;
    std::shared_ptr<VfsDirectory> CreateSubdirectory(const std::string& name) override;
    std::shared_ptr<VfsFile> CreateFile(const std::string& name) override;
    bool DeleteSubdirectory(const std::string& name) override;
    bool DeleteFile(const std::string& name) override;
    bool Rename(const std::string& name) override;

private:
    std::string path;
    std::string mode;
    std::vector<std::shared_ptr<VfsFile>> files;
    std::vector<std::shared_ptr<VfsDirectory>> subdirectories;
};

} // namespace FileSys
