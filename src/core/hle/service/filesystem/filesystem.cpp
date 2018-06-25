// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <boost/container/flat_map.hpp>
#include "common/file_util.h"
#include "core/file_sys/filesystem.h"
#include "core/file_sys/vfs.h"
#include "core/file_sys/vfs_real.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/hle/service/filesystem/fsp_srv.h"

namespace Service::FileSystem {

ResultVal<v_file> OpenRomFS() {
    return romfs;
}

/**
 * Map of registered file systems, identified by type. Once an file system is registered here, it
 * is never removed until UnregisterFileSystems is called.
 */
static boost::container::flat_map<Type, v_dir> filesystem_map;
static v_file filesystem_romfs;

ResultCode RegisterFileSystem(v_dir factory, Type type) {
    auto result = filesystem_map.emplace(type, factory);

    bool inserted = result.second;
    ASSERT_MSG(inserted, "Tried to register more than one system with same id code");

    auto& filesystem = result.first->second;
    LOG_DEBUG(Service_FS, "Registered file system {} with id code 0x{:08X}", filesystem->GetName(),
              static_cast<u32>(type));
    return RESULT_SUCCESS;
}

ResultCode RegisterRomFS(v_file filesystem) {
    bool inserted = filesystem_romfs == nullptr;
    ASSERT_MSG(inserted, "Tried to register more than one system with same id code");

    filesystem_romfs = filesystem;
    NGLOG_DEBUG(Service_FS, "Registered file system {} with id code 0x{:08X}",
                filesystem->GetName(), static_cast<u32>(Type::RomFS));
    return RESULT_SUCCESS;
}

ResultVal<v_dir> OpenFileSystem(Type type) {
    NGLOG_TRACE(Service_FS, "Opening FileSystem with type={}", static_cast<u32>(type));

    auto itr = filesystem_map.find(type);
    if (itr == filesystem_map.end()) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    return MakeResult(itr->second);
}

ResultCode FormatFileSystem(Type type) {
    LOG_TRACE(Service_FS, "Formatting FileSystem with type={}", static_cast<u32>(type));

    auto itr = filesystem_map.find(type);
    if (itr == filesystem_map.end()) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    return itr->second->GetParentDirectory()->DeleteSubdirectory(itr->second->GetName())
               ? RESULT_SUCCESS
               : ResultCode(-1);
}

void RegisterFileSystems() {
    filesystem_map.clear();

    std::string nand_directory = FileUtil::GetUserPath(D_NAND_IDX);
    std::string sd_directory = FileUtil::GetUserPath(D_SDMC_IDX);

    auto savedata =
        std::make_unique<FileSys::RealVfsDirectory>(nand_directory, filesystem::perms::all);
    RegisterFileSystem(std::move(savedata), Type::SaveData);

    auto sdcard = std::make_unique<FileSys::RealVfsDirectory>(sd_directory, filesystem::perms::all);
    RegisterFileSystem(std::move(sdcard), Type::SDMC);
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    RegisterFileSystems();
    std::make_shared<FSP_SRV>()->InstallAsService(service_manager);
}

} // namespace Service::FileSystem
