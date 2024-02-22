// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// TODO: Properly credit LibHac before this gets merged

#pragma once

#include "common/assert.h"
#include "common/common_types.h"
#include "core/file_sys/savedata_factory.h"

namespace FileSys::FsSrv::Impl {

class SaveDataProperties {
public:
    static constexpr s64 DefaultSaveDataBlockSize = 0x4000;
    static constexpr s64 BcatSaveDataJournalSize = 0x200000;

    static bool IsJournalingSupported(SaveDataFormatType type) {
        switch (type) {
        case SaveDataFormatType::Normal:
            return true;
        case SaveDataFormatType::NoJournal:
            return false;
        default:
            UNREACHABLE();
            return false;
        }
    }

    static bool IsJournalingSupported(SaveDataType type) {
        switch (type) {
        case SaveDataType::SystemSaveData:
        case SaveDataType::SaveData:
        case SaveDataType::BcatDeliveryCacheStorage:
        case SaveDataType::DeviceSaveData:
        case SaveDataType::CacheStorage:
            return true;
        case SaveDataType::TemporaryStorage:
            return false;
        default:
            UNREACHABLE();
            return false;
        }
    }

    static bool IsMultiCommitSupported(SaveDataType type) {
        switch (type) {
        case SaveDataType::SystemSaveData:
        case SaveDataType::SaveData:
        case SaveDataType::DeviceSaveData:
            return true;
        case SaveDataType::BcatDeliveryCacheStorage:
        case SaveDataType::TemporaryStorage:
        case SaveDataType::CacheStorage:
            return false;
        default:
            UNREACHABLE();
            return false;
        }
    }

    static bool IsSharedOpenNeeded(SaveDataType type) {
        switch (type) {
        case SaveDataType::SystemSaveData:
        case SaveDataType::BcatDeliveryCacheStorage:
        case SaveDataType::TemporaryStorage:
        case SaveDataType::CacheStorage:
            return false;
        case SaveDataType::SaveData:
        case SaveDataType::DeviceSaveData:
            return true;
        default:
            UNREACHABLE();
            return false;
        }
    }

    static bool CanUseIndexerReservedArea(SaveDataType type) {
        switch (type) {
        case SaveDataType::SystemSaveData:
            return true;
        case SaveDataType::SaveData:
        case SaveDataType::BcatDeliveryCacheStorage:
        case SaveDataType::DeviceSaveData:
        case SaveDataType::TemporaryStorage:
        case SaveDataType::CacheStorage:
            return false;
        default:
            UNREACHABLE();
            return false;
        }
    }

    static bool IsSystemSaveData(SaveDataType type) {
        switch (type) {
        case SaveDataType::SystemSaveData:
            return true;
        case SaveDataType::SaveData:
        case SaveDataType::BcatDeliveryCacheStorage:
        case SaveDataType::DeviceSaveData:
        case SaveDataType::TemporaryStorage:
        case SaveDataType::CacheStorage:
            return false;
        default:
            UNREACHABLE();
            return false;
        }
    }

    static bool IsObsoleteSystemSaveData(const SaveDataInfo& info) {
        constexpr std::array<u64, 2> ObsoleteSystemSaveDataIdList = {0x8000000000000060,
                                                                     0x8000000000000075};
        return std::find(ObsoleteSystemSaveDataIdList.begin(), ObsoleteSystemSaveDataIdList.end(),
                         info.save_id) != ObsoleteSystemSaveDataIdList.end();
    }

    static bool IsWipingNeededAtCleanUp(const SaveDataInfo& info) {
        switch (info.type) {
        case SaveDataType::SystemSaveData:
            break;
        case SaveDataType::SaveData:
        case SaveDataType::BcatDeliveryCacheStorage:
        case SaveDataType::DeviceSaveData:
        case SaveDataType::TemporaryStorage:
        case SaveDataType::CacheStorage:
            return true;
        default:
            UNREACHABLE();
            break;
        }

        constexpr std::array<u64, 28> SystemSaveDataIdWipingExceptionList = {
            0x8000000000000040, 0x8000000000000041, 0x8000000000000043, 0x8000000000000044,
            0x8000000000000045, 0x8000000000000046, 0x8000000000000047, 0x8000000000000048,
            0x8000000000000049, 0x800000000000004A, 0x8000000000000070, 0x8000000000000071,
            0x8000000000000072, 0x8000000000000074, 0x8000000000000076, 0x8000000000000077,
            0x8000000000000090, 0x8000000000000091, 0x8000000000000092, 0x80000000000000B0,
            0x80000000000000C1, 0x80000000000000C2, 0x8000000000000120, 0x8000000000000121,
            0x8000000000000180, 0x8000000000010003, 0x8000000000010004};

        return std::find(SystemSaveDataIdWipingExceptionList.begin(),
                         SystemSaveDataIdWipingExceptionList.end(),
                         info.save_id) == SystemSaveDataIdWipingExceptionList.end();
    }

    static bool IsValidSpaceIdForSaveDataMover(SaveDataType type, SaveDataSpaceId space_id) {
        switch (type) {
        case SaveDataType::SystemSaveData:
        case SaveDataType::SaveData:
        case SaveDataType::BcatDeliveryCacheStorage:
        case SaveDataType::DeviceSaveData:
        case SaveDataType::TemporaryStorage:
            return false;
        case SaveDataType::CacheStorage:
            return space_id == SaveDataSpaceId::NandUser || space_id == SaveDataSpaceId::SdCardUser;
        default:
            UNREACHABLE();
            return false;
        }
    }

    static bool IsReconstructible(SaveDataType type, SaveDataSpaceId space_id) {
        switch (space_id) {
        case SaveDataSpaceId::NandSystem:
        case SaveDataSpaceId::NandUser:
        case SaveDataSpaceId::ProperSystem:
        case SaveDataSpaceId::SafeMode:
            switch (type) {
            case SaveDataType::SystemSaveData:
            case SaveDataType::SaveData:
            case SaveDataType::DeviceSaveData:
                return false;
            case SaveDataType::BcatDeliveryCacheStorage:
            case SaveDataType::TemporaryStorage:
            case SaveDataType::CacheStorage:
                return true;
            default:
                UNREACHABLE();
                return false;
            }
        case SaveDataSpaceId::SdCardSystem:
        case SaveDataSpaceId::TemporaryStorage:
        case SaveDataSpaceId::SdCardUser:
            return true;
        default:
            UNREACHABLE();
            return false;
        }
    }
};

} // namespace FileSys::FsSrv::Impl
