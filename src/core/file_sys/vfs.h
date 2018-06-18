// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <vector>
#include "boost/optional.hpp"
#include "common/common_types.h"

namespace FileSys {
struct VfsDirectory;

struct VfsFile : NonCopyable {
    virtual bool IsReady() = 0;
    virtual bool IsGood() = 0;
    virtual operator bool();
    virtual void ResetState() = 0;

    virtual std::string GetName() = 0;
    virtual u64 GetSize() = 0;
    virtual bool Resize(u64 new_size) = 0;
    virtual std::shared_ptr<VfsDirectory> GetContainingDirectory() = 0;

    virtual bool IsWritable() = 0;
    virtual bool IsReadable() = 0;

    virtual boost::optional<u8> ReadByte(u64 offset);
    virtual std::vector<u8> ReadBytes(u64 offset, u64 length) = 0;

    template <typename T>
    u64 ReadBytes(T* data, u64 offset, u64 length) {
        static_assert(std::is_trivially_copyable<T>(),
                      "Given array does not consist of trivially copyable objects");
        return ReadArray<u8>(reinterpret_cast<u8*>(data), offset, length);
    }

    template <typename T>
    std::vector<T> ReadArray(u64 offset, u64 number_elements) {
        static_assert(std::is_trivially_copyable<T>(),
                      "Given array does not consist of trivially copyable objects");

        auto vec = ReadBytes(offset, number_elements * sizeof(T));
        std::vector<T> out_vec(number_elements);
        memcpy(out_vec.data(), vec.data(), vec.size());
        return out_vec;
    }

    template <typename T>
    u64 ReadArray(T* data, u64 offset, u64 number_elements) {
        static_assert(std::is_trivially_copyable<T>(),
                      "Given array does not consist of trivially copyable objects");

        std::vector<T> vec = ReadArray<T>(offset, number_elements);
        for (size_t i = 0; i < vec.size(); ++i)
            data[i] = vec[i];

        return vec.size();
    }

    virtual std::vector<u8> ReadAllBytes();

    virtual u64 WriteBytes(const std::vector<u8>& data, u64 offset) = 0;
    virtual u64 ReplaceBytes(const std::vector<u8>& data);

    virtual bool Rename(const std::string& name) = 0;

    ~VfsFile();
};

struct VfsDirectory : NonCopyable {
    virtual bool IsReady() = 0;
    virtual bool IsGood() = 0;
    virtual operator bool();
    virtual void ResetState() = 0;

    virtual std::vector<std::shared_ptr<VfsFile>> GetFiles() = 0;
    virtual std::shared_ptr<VfsFile> GetFile(const std::string& name);

    virtual std::vector<std::shared_ptr<VfsDirectory>> GetSubdirectories() = 0;
    virtual std::shared_ptr<VfsDirectory> GetSubdirectory(const std::string& name);

    virtual bool IsWritable() = 0;
    virtual bool IsReadable() = 0;

    virtual bool IsRoot() = 0;

    virtual std::string GetName() = 0;
    virtual u64 GetSize();
    virtual std::shared_ptr<VfsDirectory> GetParentDirectory() = 0;

    virtual std::shared_ptr<VfsDirectory> CreateSubdirectory(const std::string& name) = 0;
    virtual std::shared_ptr<VfsFile> CreateFile(const std::string& name) = 0;

    virtual bool DeleteSubdirectory(const std::string& name) = 0;
    virtual bool DeleteFile(const std::string& name) = 0;

    virtual bool Rename(const std::string& name) = 0;

    virtual bool Copy(const std::string& src, const std::string& dest);

    ~VfsDirectory();
};
} // namespace FileSys
