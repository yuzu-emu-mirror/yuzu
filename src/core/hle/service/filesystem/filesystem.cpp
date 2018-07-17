// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <boost/container/flat_map.hpp>
#include "common/file_util.h"
#include "core/file_sys/errors.h"
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
static std::unique_ptr<FileSys::RomFSFactory> romfs_factory;
static std::unique_ptr<FileSys::SaveDataFactory> save_data_factory;
static std::unique_ptr<FileSys::SDMCFactory> sdmc_factory;

ResultCode RegisterRomFS(std::unique_ptr<FileSys::RomFSFactory>&& factory) {
    ASSERT_MSG(romfs_factory == nullptr, "Tried to register a second RomFS");
    romfs_factory = std::move(factory);
    LOG_DEBUG(Service_FS, "Registered RomFS");
    return RESULT_SUCCESS;
}

ResultCode RegisterSaveData(std::unique_ptr<FileSys::SaveDataFactory>&& factory) {
    ASSERT_MSG(romfs_factory == nullptr, "Tried to register a second save data");
    save_data_factory = std::move(factory);
    LOG_DEBUG(Service_FS, "Registered save data");
    return RESULT_SUCCESS;
}

ResultCode RegisterSDMC(std::unique_ptr<FileSys::SDMCFactory>&& factory) {
    ASSERT_MSG(sdmc_factory == nullptr, "Tried to register a second SDMC");
    sdmc_factory = std::move(factory);
    LOG_DEBUG(Service_FS, "Registered SDMC");
    return RESULT_SUCCESS;
}

ResultVal<std::unique_ptr<FileSys::FileSystemBackend>> OpenRomFS(u64 title_id) {
    LOG_TRACE(Service_FS, "Opening RomFS for title_id={:016X}", title_id);

    if (romfs_factory == nullptr) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    return romfs_factory->Open(title_id);
}

ResultVal<std::unique_ptr<FileSys::FileSystemBackend>> OpenSaveData(
    FileSys::SaveDataSpaceId space, FileSys::SaveDataDescriptor save_struct) {
    LOG_TRACE(Service_FS, "Opening Save Data for space_id={:01X}, save_struct={}",
              static_cast<u8>(space), SaveStructDebugInfo(save_struct));

    if (save_data_factory == nullptr) {
        return ResultCode(ErrorModule::FS, FileSys::ErrCodes::SaveDataNotFound);
    }

    return save_data_factory->Open(space, save_struct);
}

ResultVal<std::unique_ptr<FileSys::FileSystemBackend>> OpenSDMC() {
    LOG_TRACE(Service_FS, "Opening SDMC");

    if (sdmc_factory == nullptr) {
        return ResultCode(ErrorModule::FS, FileSys::ErrCodes::SdCardNotFound);
    }

    return sdmc_factory->Open();
}

void RegisterFileSystems() {
    romfs_factory = nullptr;
    save_data_factory = nullptr;
    sdmc_factory = nullptr;

    std::string nand_directory = FileUtil::GetUserPath(D_NAND_IDX);
    std::string sd_directory = FileUtil::GetUserPath(D_SDMC_IDX);

    auto savedata = std::make_unique<FileSys::SaveDataFactory>(std::move(nand_directory));
    save_data_factory = std::move(savedata);

    auto sdcard = std::make_unique<FileSys::SDMCFactory>(std::move(sd_directory));
    sdmc_factory = std::move(sdcard);
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    RegisterFileSystems();
    std::make_shared<FSP_SRV>()->InstallAsService(service_manager);
}

} // namespace Service::FileSystem
