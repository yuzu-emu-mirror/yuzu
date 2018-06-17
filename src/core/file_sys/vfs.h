// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "boost/optional.hpp"
#include "common/common_types.h"
#include <string>
#include <vector>

namespace FileSys
{
    struct VfsDirectory;

    struct VfsFile : NonCopyable
    {
        bool IsReady();
        bool IsGood();
        operator bool();
        void ResetState();

        std::string GetName();
        u64 GetSize();
        bool Resize(u64 new_size);
        boost::optional<VfsDirectory> GetContainingDirectory();

        bool IsWritable();
        bool IsReadable();

        std::vector<u8> ReadBytes(u64 offset, u64 length);
        template <typename T> u64 ReadBytes(T* data, u64 offset, u64 length);

        template <typename T> std::vector<T> ReadArray(u64 offset, u64 number_elements);
        template <typename T> u64 ReadArray(T* data, u64 offset, u64 number_elements);

        std::vector<u8> ReadAllBytes();

        void WriteBytes(const std::vector<u8>& data, u64 offset);
        void ReplaceBytes(const std::vector<u8>& data);

        bool Rename(const std::string& name);
    };

    struct VfsDirectory : NonCopyable
    {
        std::vector<VfsFile> GetFiles();
        boost::optional<VfsFile> GetFile(const std::string& name);

        std::vector<VfsDirectory> GetSubdirectories();
        boost::optional<VfsFile> GetSubdirectory(const std::string& name);

        bool IsWritable();
        bool IsReadable();

        bool IsRoot();

        std::string GetName();
        u64 GetSize();
        boost::optional<VfsDirectory> GetParentDirectory();

        boost::optional<VfsDirectory> CreateSubdirectory(const std::string& name);
        boost::optional<VfsFile> CreateFile(const std::string& name);

        bool DeleteSubdirectory(const std::string& name);
        bool DeleteFile(const std::string& name);

        bool Rename(const std::string& name);

        bool Copy(const std::string& src, const std::string& dest);
    };
}
