// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once
#include "vfs.h"

namespace FileSys {

struct OffsetVfsFile : public VfsFile {
    OffsetVfsFile(VfsFile&& file, u64 offset, u64 size);

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
    std::unique_ptr<VfsFile> file;
    u64 offset;
    u64 size;
};

} // namespace FileSys
