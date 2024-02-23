// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include "common/common_types.h"
#include "core/file_sys/fs_save_data_types.h"
#include "core/hle/service/cmif_types.h"
#include "core/hle/service/service.h"

namespace Service::FileSystem {

class SaveDataController;

class ISaveDataInfoReader final : public ServiceFramework<ISaveDataInfoReader> {
public:
    explicit ISaveDataInfoReader(Core::System& system_,
                                 std::shared_ptr<SaveDataController> save_data_controller_,
                                 FileSys::SaveDataSpaceId space);
    ~ISaveDataInfoReader() override;

    Result ReadSaveDataInfo(Out<u64> out_count,
                            OutArray<FileSys::SaveDataInfo, BufferAttr_HipcMapAlias> out_entries);

private:
    void FindAllSaves(FileSys::SaveDataSpaceId space);
    void FindNormalSaves(FileSys::SaveDataSpaceId space, const FileSys::VirtualDir& type);
    void FindTemporaryStorageSaves(FileSys::SaveDataSpaceId space, const FileSys::VirtualDir& type);

    std::shared_ptr<SaveDataController> save_data_controller;
    std::vector<FileSys::SaveDataInfo> info;
    u64 next_entry_index = 0;
};

} // namespace Service::FileSystem
