// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/file_sys/vfs_ro_layer.h"

namespace FileSys {

ReadOnlyVfsFileLayer::ReadOnlyVfsFileLayer(VirtualFile base) : base(std::move(base)) {}

ReadOnlyVfsFileLayer::~ReadOnlyVfsFileLayer() = default;

std::string ReadOnlyVfsFileLayer::GetName() const {
    return base->GetName();
}

std::size_t ReadOnlyVfsFileLayer::GetSize() const {
    return base->GetSize();
}

bool ReadOnlyVfsFileLayer::Resize(std::size_t new_size) {
    return false;
}

std::shared_ptr<VfsDirectory> ReadOnlyVfsFileLayer::GetContainingDirectory() const {
    // Make containing read-only to prevent escaping the layer by getting containing and then
    // getting this file again.
    return std::make_shared<ReadOnlyVfsDirectoryLayer>(base->GetContainingDirectory());
}

bool ReadOnlyVfsFileLayer::IsWritable() const {
    return false;
}

bool ReadOnlyVfsFileLayer::IsReadable() const {
    return base->IsReadable();
}

std::size_t ReadOnlyVfsFileLayer::Read(u8* data, std::size_t length, std::size_t offset) const {
    return base->Read(data, length, offset);
}

std::size_t ReadOnlyVfsFileLayer::Write(const u8* data, std::size_t length, std::size_t offset) {
    return 0;
}

bool ReadOnlyVfsFileLayer::Rename(std::string_view name) {
    return false;
}

std::string ReadOnlyVfsFileLayer::GetFullPath() const {
    return base->GetFullPath();
}

ReadOnlyVfsDirectoryLayer::ReadOnlyVfsDirectoryLayer(VirtualDir base) : base(std::move(base)) {}

ReadOnlyVfsDirectoryLayer::~ReadOnlyVfsDirectoryLayer() = default;

std::vector<std::shared_ptr<VfsFile>> ReadOnlyVfsDirectoryLayer::GetFiles() const {
    std::vector<VirtualFile> out;
    const auto in = base->GetFiles();
    std::transform(in.begin(), in.end(), std::back_inserter(out),
                   [](const VirtualFile& i) { return std::make_shared<ReadOnlyVfsFileLayer>(i); });
    return out;
}

std::vector<std::shared_ptr<VfsDirectory>> ReadOnlyVfsDirectoryLayer::GetSubdirectories() const {
    std::vector<VirtualDir> out;
    const auto in = base->GetSubdirectories();
    std::transform(in.begin(), in.end(), std::back_inserter(out), [](const VirtualDir& i) {
        return std::make_shared<ReadOnlyVfsDirectoryLayer>(i);
    });
    return out;
}

std::string ReadOnlyVfsDirectoryLayer::GetName() const {
    return base->GetName();
}

std::shared_ptr<VfsDirectory> ReadOnlyVfsDirectoryLayer::GetParentDirectory() const {
    return std::make_shared<ReadOnlyVfsDirectoryLayer>(base->GetParentDirectory());
}

std::shared_ptr<VfsFile> ReadOnlyVfsDirectoryLayer::GetFileRelative(std::string_view path) const {
    return std::make_shared<ReadOnlyVfsFileLayer>(base->GetFileRelative(path));
}

std::shared_ptr<VfsFile> ReadOnlyVfsDirectoryLayer::GetFileAbsolute(std::string_view path) const {
    return std::make_shared<ReadOnlyVfsFileLayer>(base->GetFileAbsolute(path));
}

std::shared_ptr<VfsDirectory> ReadOnlyVfsDirectoryLayer::GetDirectoryRelative(
    std::string_view path) const {
    return std::make_shared<ReadOnlyVfsDirectoryLayer>(base->GetDirectoryRelative(path));
}

std::shared_ptr<VfsDirectory> ReadOnlyVfsDirectoryLayer::GetDirectoryAbsolute(
    std::string_view path) const {
    return std::make_shared<ReadOnlyVfsDirectoryLayer>(base->GetDirectoryAbsolute(path));
}

std::shared_ptr<VfsFile> ReadOnlyVfsDirectoryLayer::GetFile(std::string_view name) const {
    return std::make_shared<ReadOnlyVfsFileLayer>(base->GetFile(name));
}

std::shared_ptr<VfsDirectory> ReadOnlyVfsDirectoryLayer::GetSubdirectory(
    std::string_view name) const {
    return std::make_shared<ReadOnlyVfsDirectoryLayer>(base->GetSubdirectory(name));
}

bool ReadOnlyVfsDirectoryLayer::IsRoot() const {
    return base->IsRoot();
}

std::size_t ReadOnlyVfsDirectoryLayer::GetSize() const {
    return base->GetSize();
}

bool ReadOnlyVfsDirectoryLayer::Copy(std::string_view src, std::string_view dest) {
    return false;
}

std::string ReadOnlyVfsDirectoryLayer::GetFullPath() const {
    return base->GetFullPath();
}

bool ReadOnlyVfsDirectoryLayer::IsWritable() const {
    return false;
}

bool ReadOnlyVfsDirectoryLayer::IsReadable() const {
    return base->IsReadable();
}

std::shared_ptr<VfsDirectory> ReadOnlyVfsDirectoryLayer::CreateSubdirectory(std::string_view name) {
    return nullptr;
}

std::shared_ptr<VfsFile> ReadOnlyVfsDirectoryLayer::CreateFile(std::string_view name) {
    return nullptr;
}

std::shared_ptr<VfsFile> ReadOnlyVfsDirectoryLayer::CreateFileAbsolute(std::string_view path) {
    return nullptr;
}

std::shared_ptr<VfsFile> ReadOnlyVfsDirectoryLayer::CreateFileRelative(std::string_view path) {
    return nullptr;
}

std::shared_ptr<VfsDirectory> ReadOnlyVfsDirectoryLayer::CreateDirectoryAbsolute(
    std::string_view path) {
    return nullptr;
}

std::shared_ptr<VfsDirectory> ReadOnlyVfsDirectoryLayer::CreateDirectoryRelative(
    std::string_view path) {
    return nullptr;
}

bool ReadOnlyVfsDirectoryLayer::DeleteSubdirectory(std::string_view name) {
    return false;
}

bool ReadOnlyVfsDirectoryLayer::DeleteSubdirectoryRecursive(std::string_view name) {
    return false;
}

bool ReadOnlyVfsDirectoryLayer::CleanSubdirectoryRecursive(std::string_view name) {
    return false;
}

bool ReadOnlyVfsDirectoryLayer::DeleteFile(std::string_view name) {
    return false;
}

bool ReadOnlyVfsDirectoryLayer::Rename(std::string_view name) {
    return false;
}

} // namespace FileSys