// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/file_sys/vfs_offset.h"

namespace FileSys {

OffsetVfsFile::OffsetVfsFile(VfsFile&& file, u64 offset, u64 size)
    : file(std::make_unique<VfsFile>(file)), offset(offset), size(size) {}

bool OffsetVfsFile::IsReady() {
    return file->IsReady();
}

bool OffsetVfsFile::IsGood() {
    return file->IsGood();
}

void OffsetVfsFile::ResetState() {
    file->ResetState();
}

std::string OffsetVfsFile::GetName() {
    return file->GetName();
}

u64 OffsetVfsFile::GetSize() {
    return size;
}

bool OffsetVfsFile::Resize(u64 new_size) {
    if (offset + new_size < file->GetSize()) {
        size = new_size;
        return true;
    }

    return false;
}

std::shared_ptr<VfsDirectory> OffsetVfsFile::GetContainingDirectory() {
    return file->GetContainingDirectory();
}

bool OffsetVfsFile::IsWritable() {
    return file->IsWritable();
}

bool OffsetVfsFile::IsReadable() {
    return file->IsReadable();
}

std::vector<u8> OffsetVfsFile::ReadBytes(u64 r_offset, u64 r_length) {
    return file->ReadBytes(offset + r_offset, std::min(r_offset + r_length, size));
}

u64 OffsetVfsFile::WriteBytes(const std::vector<u8>& data, u64 r_offset) {
    auto end = data.end();
    if (data.size() + r_offset > size)
        end = data.begin + size - r_offset;

    return file->WriteBytes(std::vector<u8>(data.begin(), end), r_offset + offset);
}

bool OffsetVfsFile::Rename(const std::string& name) {
    return file->Rename(name);
}

} // namespace FileSys
