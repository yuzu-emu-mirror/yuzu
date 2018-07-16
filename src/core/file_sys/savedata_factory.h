// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include "common/common_types.h"
#include "core/file_sys/filesystem.h"
#include "core/hle/result.h"

namespace FileSys {

enum class SaveDataSpaceId : u8 { NandSystem = 0, NandUser = 1, SdCard = 2, TemporaryStorage = 3 };

enum class SaveDataType : u8 {
    SystemSaveData = 0,
    SaveData = 1,
    BcatDeliveryCacheStorage = 2,
    DeviceSaveData = 3,
    TemporaryStorage = 4,
    CacheStorage = 5
};

struct SaveStruct {
    u64_le title_id;
    u128 user_id;
    u64_le save_id;
    SaveDataType type;
    INSERT_PADDING_BYTES(7);
    u64_le zero_1;
    u64_le zero_2;
    u64_le zero_3;
};
static_assert(sizeof(SaveStruct) == 0x40, "SaveStruct has incorrect size.");

std::string SaveStructDebugInfo(SaveStruct save_struct);

/// File system interface to the SaveData archive
// nand_directory should be the root of nand, sd_directory should be the root of sd
class SaveDataFactory {
public:
    SaveDataFactory(std::string nand_directory);

    ResultVal<std::unique_ptr<FileSystemBackend>> Open(SaveDataSpaceId space, SaveStruct meta);
    ResultCode Format(SaveDataSpaceId space, SaveStruct meta);

private:
    std::string nand_directory;
    std::string sd_directory;

    std::string GetFullPath(SaveDataSpaceId space, SaveDataType type, u64 title_id, u128 user_id,
                            u64 save_id) const;
};

} // namespace FileSys
