// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/file_sys/disk_filesystem.h"
#include "core/file_sys/savedata_factory.h"
#include "core/hle/kernel/process.h"

namespace FileSys {

std::string SaveStructDebugInfo(SaveStruct save_struct) {
    return fmt::format("[type={:02X}, title_id={:016X}, user_id={:016X}{:016X}, save_id={:016X}]",
                       static_cast<u8>(save_struct.type), save_struct.title_id,
                       save_struct.user_id[1], save_struct.user_id[0], save_struct.save_id);
}

SaveDataFactory::SaveDataFactory(std::string save_directory)
    : save_directory(std::move(save_directory)) {}

ResultVal<std::unique_ptr<FileSystemBackend>> SaveDataFactory::Open(SaveDataSpaceId space,

                                                                    SaveStruct meta) {
    std::string save_directory =
        GetFullPath(space, meta.type, meta.title_id, meta.user_id, meta.save_id);

    if (!FileUtil::Exists(save_directory)) {
        // TODO(bunnei): This is a work-around to always create a save data directory if it does not
        // already exist. This is a hack, as we do not understand yet how this works on hardware.
        // Without a save data directory, many games will assert on boot. This should not have any
        // bad side-effects.
        FileUtil::CreateFullPath(save_directory);
    }

    // Return an error if the save data doesn't actually exist.
    if (!FileUtil::IsDirectory(save_directory)) {
        // TODO(Subv): Find out correct error code.
        return ResultCode(-1);
    }

    auto archive = std::make_unique<Disk_FileSystem>(save_directory);
    return MakeResult<std::unique_ptr<FileSystemBackend>>(std::move(archive));
}

ResultCode SaveDataFactory::Format(SaveDataSpaceId space, SaveStruct meta) {
    LOG_WARNING(Service_FS, "Formatting save data of space={:01X}, meta={}", static_cast<u8>(space),
                SaveStructDebugInfo(meta));
    // Create the save data directory.
    if (!FileUtil::CreateFullPath(
            GetFullPath(space, meta.type, meta.title_id, meta.user_id, meta.save_id))) {
        // TODO(Subv): Find the correct error code.
        return ResultCode(-1);
    }

    return RESULT_SUCCESS;
}

std::string SaveDataFactory::GetFullPath(SaveDataSpaceId space, SaveDataType type, u64 title_id,
                                         u128 user_id, u64 save_id) const {
    static std::vector<std::string> space_names = {"sysnand", "usrnand", "sd", "temp"};
    static std::vector<std::string> type_names = {"system", "user", "bcat",
                                                  "device", "temp", "cache"};

    if (type == SaveDataType::SaveData && title_id == 0)
        title_id = Core::CurrentProcess()->program_id;

    return fmt::format("{}{}/{}/{:016X}/{:016X}{:016X}/{:016X}", save_directory,
                       space_names[static_cast<u8>(space)], type_names[static_cast<u8>(type)],
                       title_id, user_id[1], user_id[0], save_id);
}

} // namespace FileSys
