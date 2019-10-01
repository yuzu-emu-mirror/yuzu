// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "core/file_sys/vfs.h"

namespace FileSys {

// Class that wraps a VfsFile making it read-only
class ReadOnlyVfsFileLayer : public VfsFile {
public:
    explicit ReadOnlyVfsFileLayer(VirtualFile base);
    ~ReadOnlyVfsFileLayer() override;

    std::string GetName() const override;
    std::size_t GetSize() const override;
    bool Resize(std::size_t new_size) override;
    std::shared_ptr<VfsDirectory> GetContainingDirectory() const override;
    bool IsWritable() const override;
    bool IsReadable() const override;
    std::size_t Read(u8* data, std::size_t length, std::size_t offset) const override;
    std::size_t Write(const u8* data, std::size_t length, std::size_t offset) override;
    bool Rename(std::string_view name) override;

    std::string GetFullPath() const override;

private:
    VirtualFile base;
};

// Class that wraps a VfsDirectory making it and its children read only.
class ReadOnlyVfsDirectoryLayer : public ReadOnlyVfsDirectory {
public:
    explicit ReadOnlyVfsDirectoryLayer(VirtualDir base);
    ~ReadOnlyVfsDirectoryLayer() override;

    std::vector<std::shared_ptr<VfsFile>> GetFiles() const override;
    std::vector<std::shared_ptr<VfsDirectory>> GetSubdirectories() const override;
    std::string GetName() const override;
    std::shared_ptr<VfsDirectory> GetParentDirectory() const override;

    std::shared_ptr<VfsFile> GetFileRelative(std::string_view path) const override;
    std::shared_ptr<VfsFile> GetFileAbsolute(std::string_view path) const override;
    std::shared_ptr<VfsDirectory> GetDirectoryRelative(std::string_view path) const override;
    std::shared_ptr<VfsDirectory> GetDirectoryAbsolute(std::string_view path) const override;
    std::shared_ptr<VfsFile> GetFile(std::string_view name) const override;
    std::shared_ptr<VfsDirectory> GetSubdirectory(std::string_view name) const override;
    bool IsRoot() const override;
    std::size_t GetSize() const override;
    bool Copy(std::string_view src, std::string_view dest) override;
    std::string GetFullPath() const override;
    bool IsWritable() const override;
    bool IsReadable() const override;
    std::shared_ptr<VfsDirectory> CreateSubdirectory(std::string_view name) override;
    std::shared_ptr<VfsFile> CreateFile(std::string_view name) override;
    std::shared_ptr<VfsFile> CreateFileAbsolute(std::string_view path) override;
    std::shared_ptr<VfsFile> CreateFileRelative(std::string_view path) override;
    std::shared_ptr<VfsDirectory> CreateDirectoryAbsolute(std::string_view path) override;
    std::shared_ptr<VfsDirectory> CreateDirectoryRelative(std::string_view path) override;
    bool DeleteSubdirectory(std::string_view name) override;
    bool DeleteSubdirectoryRecursive(std::string_view name) override;
    bool CleanSubdirectoryRecursive(std::string_view name) override;
    bool DeleteFile(std::string_view name) override;
    bool Rename(std::string_view name) override;

private:
    VirtualDir base;
};

} // namespace FileSys