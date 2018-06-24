// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/result.h"

namespace FileSys {

class StorageBackend;
class DirectoryBackend;

// Path string type
enum LowPathType : u32 {
    Invalid = 0,
    Empty = 1,
    Binary = 2,
    Char = 3,
    Wchar = 4,
};

enum EntryType : u8 {
    Directory = 0,
    File = 1,
};

enum class Mode : u32 {
    Read = 1,
    Write = 2,
    Append = 4,
};

class Path {
public:
    Path() : type(Invalid) {}
    Path(const char* path) : type(Char), string(path) {}
    Path(std::vector<u8> binary_data) : type(Binary), binary(std::move(binary_data)) {}
    Path(LowPathType type, u32 size, u32 pointer);

    LowPathType GetType() const {
        return type;
    }

    /**
     * Gets the string representation of the path for debugging
     * @return String representation of the path for debugging
     */
    std::string DebugStr() const;

    std::string AsString() const;
    std::u16string AsU16Str() const;
    std::vector<u8> AsBinary() const;

private:
    LowPathType type;
    std::vector<u8> binary;
    std::string string;
    std::u16string u16str;
};

/// Parameters of the archive, as specified in the Create or Format call.
struct ArchiveFormatInfo {
    u32_le total_size;         ///< The pre-defined size of the archive.
    u32_le number_directories; ///< The pre-defined number of directories in the archive.
    u32_le number_files;       ///< The pre-defined number of files in the archive.
    u8 duplicate_data;         ///< Whether the archive should duplicate the data.
};
static_assert(std::is_pod<ArchiveFormatInfo>::value, "ArchiveFormatInfo is not POD");

class FileSystemFactory : NonCopyable {
public:
    virtual ~FileSystemFactory() {}

    /**
     * Get a descriptive name for the archive (e.g. "RomFS", "SaveData", etc.)
     */
    virtual std::string GetName() const = 0;

    /**
     * Tries to open the archive of this type with the specified path
     * @param path Path to the archive
     * @return An ArchiveBackend corresponding operating specified archive path.
     */
    virtual ResultVal<std::unique_ptr<FileSystemBackend>> Open(const Path& path) = 0;

    /**
     * Deletes the archive contents and then re-creates the base folder
     * @param path Path to the archive
     * @return ResultCode of the operation, 0 on success
     */
    virtual ResultCode Format(const Path& path) = 0;

    /**
     * Retrieves the format info about the archive with the specified path
     * @param path Path to the archive
     * @return Format information about the archive or error code
     */
    virtual ResultVal<ArchiveFormatInfo> GetFormatInfo(const Path& path) const = 0;
};

} // namespace FileSys
