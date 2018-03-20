// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "common/common_types.h"
#include "core/hle/result.h"

namespace FileSys {
class FileSystemBackend;
class FileSystemFactory;
class Path;
} // namespace FileSys

namespace Service {

namespace SM {
class ServiceManager;
} // namespace SM

namespace FileSystem {

/// Supported FileSystem types
enum class Type {
    RomFS = 1,
    SaveData = 2,
    SDMC = 3,
};

/**
 * Registers a FileSystem, instances of which can later be opened using its IdCode.
 * @param factory FileSystem backend interface to use
 * @param type Type used to access this type of FileSystem
 */
ResultCode RegisterFileSystem(std::unique_ptr<FileSys::FileSystemFactory>&& factory, Type type);

/**
 * Opens a file system
 * @param type Type of the file system to open
 * @param path Path to the file system, used with Binary paths
 * @return FileSys::FileSystemBackend interface to the file system
 */
ResultVal<std::unique_ptr<FileSys::FileSystemBackend>> OpenFileSystem(Type type,
                                                                      FileSys::Path& path);

/**
 * Formats a file system
 * @param type Type of the file system to format
 * @return ResultCode of the operation
 */
ResultCode FormatFileSystem(Type type);

/// Registers all Filesystem services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace FileSystem
} // namespace Service
