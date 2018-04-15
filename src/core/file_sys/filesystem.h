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

class FileSystemBackend : NonCopyable {
public:
    virtual ~FileSystemBackend() {}

    /**
     * Get a descriptive name for the archive (e.g. "RomFS", "SaveData", etc.)
     */
    virtual std::string GetName() const = 0;

    /**
     * Create a file specified by its path
     * @param path Path relative to the Archive
     * @param size The size of the new file, filled with zeroes
     * @return Result of the operation
     */
    virtual ResultCode CreateFile(const std::string& path, u64 size) const = 0;

    /**
     * Delete a file specified by its path
     * @param path Path relative to the archive
     * @return Result of the operation
     */
    virtual ResultCode DeleteFile(const std::string& path) const = 0;

    /**
     * Create a directory specified by its path
     * @param path Path relative to the archive
     * @return Result of the operation
     */
    virtual ResultCode CreateDirectory(const std::string& path) const = 0;

    /**
     * Delete a directory specified by its path
     * @param path Path relative to the archive
     * @return Result of the operation
     */
    virtual ResultCode DeleteDirectory(const Path& path) const = 0;

    /**
     * Delete a directory specified by its path and anything under it
     * @param path Path relative to the archive
     * @return Result of the operation
     */
    virtual ResultCode DeleteDirectoryRecursively(const Path& path) const = 0;

    /**
     * Rename a File specified by its path
     * @param src_path Source path relative to the archive
     * @param dest_path Destination path relative to the archive
     * @return Result of the operation
     */
    virtual ResultCode RenameFile(const Path& src_path, const Path& dest_path) const = 0;

    /**
     * Rename a Directory specified by its path
     * @param src_path Source path relative to the archive
     * @param dest_path Destination path relative to the archive
     * @return Result of the operation
     */
    virtual ResultCode RenameDirectory(const Path& src_path, const Path& dest_path) const = 0;

    /**
     * Open a file specified by its path, using the specified mode
     * @param path Path relative to the archive
     * @param mode Mode to open the file with
     * @return Opened file, or error code
     */
    virtual ResultVal<std::unique_ptr<StorageBackend>> OpenFile(const std::string& path,
                                                                Mode mode) const = 0;

    /**
     * Open a directory specified by its path
     * @param path Path relative to the archive
     * @return Opened directory, or error code
     */
    virtual ResultVal<std::unique_ptr<DirectoryBackend>> OpenDirectory(
        const std::string& path) const = 0;

    /**
     * Get the free space
     * @return The number of free bytes in the archive
     */
    virtual u64 GetFreeSpaceSize() const = 0;

    /**
     * Get the type of the specified path
     * @return The type of the specified path or error code
     */
    virtual ResultVal<EntryType> GetEntryType(const std::string& path) const = 0;
};

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
