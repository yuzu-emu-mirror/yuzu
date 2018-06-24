// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <type_traits>
#include <vector>
#include "boost/optional.hpp"
#include "common/common_types.h"
#include "common/file_util.h"
#include "common/std_filesystem.h"

namespace FileSys {
struct VfsDirectory;

struct VfsFile : NonCopyable {
    virtual ~VfsFile();

    virtual std::string GetName() const = 0;
    virtual size_t GetSize() const = 0;
    virtual bool Resize(size_t new_size) = 0;
    virtual std::shared_ptr<VfsDirectory> GetContainingDirectory() const = 0;

    virtual bool IsWritable() const = 0;
    virtual bool IsReadable() const = 0;

    virtual size_t Read(u8* data, size_t length, size_t offset = 0) const = 0;
    virtual size_t Write(const u8* data, size_t length, size_t offset = 0) = 0;

    virtual boost::optional<u8> ReadByte(size_t offset = 0) const;
    virtual std::vector<u8> ReadBytes(size_t size, size_t offset = 0) const;
    virtual std::vector<u8> ReadAllBytes() const;

    template <typename T>
    size_t ReadArray(T* data, size_t number_elements, size_t offset = 0) const {
        static_assert(std::is_trivially_copyable<T>::value,
                      "Data type must be trivially copyable.");

        return Read(reinterpret_cast<u8*>(data), number_elements * sizeof(T), offset);
    }

    template <typename T>
    size_t ReadBytes(T* data, size_t size, size_t offset = 0) const {
        static_assert(std::is_trivially_copyable<T>::value,
                      "Data type must be trivially copyable.");
        return Read(reinterpret_cast<u8*>(data), size, offset);
    }

    template <typename T>
    size_t ReadObject(T* data, size_t offset = 0) const {
        static_assert(std::is_trivially_copyable<T>::value,
                      "Data type must be trivially copyable.");
        return Read(reinterpret_cast<u8*>(data), sizeof(T), offset);
    }

    virtual bool WriteByte(u8 data, size_t offset = 0);
    virtual size_t WriteBytes(std::vector<u8> data, size_t offset = 0);

    template <typename T>
    size_t WriteArray(T* data, size_t number_elements, size_t offset = 0) {
        static_assert(std::is_trivially_copyable<T>::value,
                      "Data type must be trivially copyable.");

        return Write(data, number_elements * sizeof(T), offset);
    }

    template <typename T>
    size_t WriteBytes(T* data, size_t size, size_t offset = 0) {
        static_assert(std::is_trivially_copyable<T>::value,
                      "Data type must be trivially copyable.");
        return Write(reinterpret_cast<u8*>(data), size, offset);
    }

    template <typename T>
    size_t WriteObject(T& data, size_t offset = 0) {
        static_assert(std::is_trivially_copyable<T>::value,
                      "Data type must be trivially copyable.");
        return Write(&data, sizeof(T), offset);
    }

    virtual bool Rename(const std::string& name) = 0;
};

struct VfsDirectory : NonCopyable {
    virtual ~VfsDirectory();

    virtual std::shared_ptr<VfsFile> GetFileRelative(const filesystem::path& path) const;
    virtual std::shared_ptr<VfsFile> GetFileAbsolute(const filesystem::path& path) const;

    virtual std::vector<std::shared_ptr<VfsFile>> GetFiles() const = 0;
    virtual std::shared_ptr<VfsFile> GetFile(const std::string& name) const;

    virtual std::vector<std::shared_ptr<VfsDirectory>> GetSubdirectories() const = 0;
    virtual std::shared_ptr<VfsDirectory> GetSubdirectory(const std::string& name) const;

    virtual bool IsWritable() const = 0;
    virtual bool IsReadable() const = 0;

    virtual bool IsRoot() const;

    virtual std::string GetName() const = 0;
    virtual size_t GetSize() const;
    virtual std::shared_ptr<VfsDirectory> GetParentDirectory() const = 0;

    virtual std::shared_ptr<VfsDirectory> CreateSubdirectory(const std::string& name) = 0;
    virtual std::shared_ptr<VfsFile> CreateFile(const std::string& name) = 0;

    virtual bool DeleteSubdirectory(const std::string& name) = 0;
    virtual bool DeleteFile(const std::string& name) = 0;

    virtual bool Rename(const std::string& name) = 0;

    virtual bool Copy(const std::string& src, const std::string& dest);
};
} // namespace FileSys
