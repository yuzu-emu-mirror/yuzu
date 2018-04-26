// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <boost/container/flat_map.hpp>
#include "common/file_util.h"
#include "core/file_sys/filesystem.h"
#include "core/file_sys/savedata_factory.h"
#include "core/file_sys/sdmc_factory.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/hle/service/filesystem/fsp_srv.h"

namespace Service::FileSystem {

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
    NGLOG_DEBUG(Service_FS, "Registered file system {} with id code {:#010X}",
                filesystem->GetName(), static_cast<u32>(type));
    return RESULT_SUCCESS;
}

ResultVal<std::unique_ptr<FileSys::FileSystemBackend>> OpenFileSystem(Type type,
                                                                      FileSys::Path& path) {
    NGLOG_TRACE(Service_FS, "Opening FileSystem with type={}", static_cast<u32>(type));

    auto itr = filesystem_map.find(type);
    if (itr == filesystem_map.end()) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    return itr->second->Open(path);
}

ResultCode FormatFileSystem(Type type) {
    NGLOG_TRACE(Service_FS, "Formatting FileSystem with type={}", static_cast<u32>(type));

    auto itr = filesystem_map.find(type);
    if (itr == filesystem_map.end()) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    FileSys::Path unused;
    return itr->second->Format(unused);
}

void RegisterFileSystems() {
    filesystem_map.clear();

    std::string nand_directory = FileUtil::GetUserPath(D_NAND_IDX);
    std::string sd_directory = FileUtil::GetUserPath(D_SDMC_IDX);

    auto savedata = std::make_unique<FileSys::SaveData_Factory>(std::move(nand_directory));
    RegisterFileSystem(std::move(savedata), Type::SaveData);

    auto sdcard = std::make_unique<FileSys::SDMC_Factory>(std::move(sd_directory));
    RegisterFileSystem(std::move(sdcard), Type::SDMC);
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    RegisterFileSystems();
    std::make_shared<FSP_SRV>()->InstallAsService(service_manager);
}

} // namespace Service::FileSystem
