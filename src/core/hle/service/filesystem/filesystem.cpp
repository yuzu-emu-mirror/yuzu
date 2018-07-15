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
static std::unique_ptr<FileSys::RomFSFactory> romfs;
static std::unique_ptr<FileSys::SaveDataFactory> save_data;
static std::unique_ptr<FileSys::SDMCFactory> sdmc;

ResultCode RegisterRomFS(std::unique_ptr<FileSys::RomFSFactory>&& factory) {
    ASSERT_MSG(romfs == nullptr, "Tried to register a second RomFS");
    romfs = std::move(factory);
    LOG_DEBUG(Service_FS, "Registered RomFS");
    return RESULT_SUCCESS;
}

ResultCode RegisterSaveData(std::unique_ptr<FileSys::SaveDataFactory>&& factory) {
    ASSERT_MSG(romfs == nullptr, "Tried to register a second save data");
    save_data = std::move(factory);
    LOG_DEBUG(Service_FS, "Registered save data");
    return RESULT_SUCCESS;
}

ResultCode RegisterSDMC(std::unique_ptr<FileSys::SDMCFactory>&& factory) {
    ASSERT_MSG(romfs == nullptr, "Tried to register a second SDMC");
    sdmc = std::move(factory);
    LOG_DEBUG(Service_FS, "Registered SDMC");
    return RESULT_SUCCESS;
}

ResultVal<std::unique_ptr<FileSys::FileSystemBackend>> OpenRomFS(u64 title_id) {
    LOG_TRACE(Service_FS, "Opening RomFS for title_id={:016X}", title_id);

    if (romfs == nullptr) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    return romfs->Open(title_id);
}

ResultVal<std::unique_ptr<FileSys::FileSystemBackend>> OpenSaveData(
    FileSys::SaveDataSpaceId space, FileSys::SaveStruct save_struct) {
    LOG_TRACE(Service_FS, "Opening Save Data for space_id={:01X}, save_struct={}",
              static_cast<u8>(space), SaveStructDebugInfo(save_struct));

    if (save_data == nullptr) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    return save_data->Open(space, save_struct);
}

ResultVal<std::unique_ptr<FileSys::FileSystemBackend>> OpenSDMC() {
    LOG_TRACE(Service_FS, "Opening SDMC");

    if (romfs == nullptr) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    return sdmc->Open();
}

ResultCode FormatSaveData(FileSys::SaveDataSpaceId space, FileSys::SaveStruct save_struct) {
    LOG_TRACE(Service_FS, "Formatting Save Data for space_id={:01X}, save_struct={}",
              static_cast<u8>(space), SaveStructDebugInfo(save_struct));

    if (save_data == nullptr) {
        // TODO(bunnei): Find a better error code for this
        return ResultCode(-1);
    }

    return save_data->Format(space, save_struct);
}

void RegisterFileSystems() {
    romfs = nullptr;
    save_data = nullptr;
    sdmc = nullptr;

    std::string save_directory = FileUtil::GetUserPath(D_SAVE_IDX);
    std::string sd_directory = FileUtil::GetUserPath(D_SDMC_IDX);

    auto savedata = std::make_unique<FileSys::SaveDataFactory>(std::move(save_directory));
    RegisterSaveData(std::move(savedata));

    auto sdcard = std::make_unique<FileSys::SDMCFactory>(std::move(sd_directory));
    RegisterSDMC(std::move(sdcard));
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    RegisterFileSystems();
    std::make_shared<FSP_SRV>()->InstallAsService(service_manager);
}

} // namespace Service::FileSystem
