// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <boost/container/flat_map.hpp>
#include "core/file_sys/filesystem.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/hle/service/filesystem/fsp_srv.h"

namespace Service {
namespace FileSystem {

/**
 * Map of registered file systems, identified by type. Once an file system is registered here, it
 * is never removed until UnregisterFileSystems is called.
 */
static boost::container::flat_map<Type, std::unique_ptr<FileSys::FileSystemFactory>> filesystem_map;

ResultCode RegisterFileSystem(std::unique_ptr<FileSys::FileSystemFactory>&& factory, Type type) {
    auto result = filesystem_map.emplace(type, std::move(factory));

    bool inserted = result.second;
    ASSERT_MSG(inserted, "Tried to register more than one system with same id code");

    auto& filesystem = result.first->second;
    LOG_DEBUG(Service_FS, "Registered file system %s with id code 0x%08X",
              filesystem->GetName().c_str(), static_cast<u32>(type));
    return RESULT_SUCCESS;
}

ResultVal<std::unique_ptr<FileSys::FileSystemBackend>> OpenFileSystem(Type type,
                                                                      FileSys::Path& path) {
    LOG_TRACE(Service_FS, "Opening FileSystem with type=%d", type);

    auto itr = filesystem_map.find(type);
    if (itr == filesystem_map.end()) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    return itr->second->Open(path);
}

void UnregisterFileSystems() {
    filesystem_map.clear();
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    UnregisterFileSystems();
    std::make_shared<FSP_SRV>()->InstallAsService(service_manager);
}

} // namespace FileSystem
} // namespace Service
