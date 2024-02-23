// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// TODO: Properly credit LibHac before this gets merged

#pragma once

#include "common/assert.h"
#include "common/common_types.h"
#include "core/file_sys/fs_save_data_types.h"

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
        case SaveDataType::System:
        case SaveDataType::Account:
        case SaveDataType::Bcat:
        case SaveDataType::Device:
        case SaveDataType::Cache:
            return true;
        case SaveDataType::Temporary:
            return false;
        default:
            UNREACHABLE();
            return false;
        }
    }

    static bool IsMultiCommitSupported(SaveDataType type) {
        switch (type) {
        case SaveDataType::System:
        case SaveDataType::Account:
        case SaveDataType::Device:
            return true;
        case SaveDataType::Bcat:
        case SaveDataType::Temporary:
        case SaveDataType::Cache:
            return false;
        default:
            UNREACHABLE();
            return false;
        }
    }

    static bool IsSharedOpenNeeded(SaveDataType type) {
        switch (type) {
        case SaveDataType::System:
        case SaveDataType::Bcat:
        case SaveDataType::Temporary:
        case SaveDataType::Cache:
            return false;
        case SaveDataType::Account:
        case SaveDataType::Device:
            return true;
        default:
            UNREACHABLE();
            return false;
        }
    }

    static bool CanUseIndexerReservedArea(SaveDataType type) {
        switch (type) {
        case SaveDataType::System:
            return true;
        case SaveDataType::Account:
        case SaveDataType::Bcat:
        case SaveDataType::Device:
        case SaveDataType::Temporary:
        case SaveDataType::Cache:
            return false;
        default:
            UNREACHABLE();
            return false;
        }
    }

    static bool IsSystemSaveData(SaveDataType type) {
        switch (type) {
        case SaveDataType::System:
            return true;
        case SaveDataType::Account:
        case SaveDataType::Bcat:
        case SaveDataType::Device:
        case SaveDataType::Temporary:
        case SaveDataType::Cache:
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
        case SaveDataType::System:
            break;
        case SaveDataType::Account:
        case SaveDataType::Bcat:
        case SaveDataType::Device:
        case SaveDataType::Temporary:
        case SaveDataType::Cache:
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
        case SaveDataType::System:
        case SaveDataType::Account:
        case SaveDataType::Bcat:
        case SaveDataType::Device:
        case SaveDataType::Temporary:
            return false;
        case SaveDataType::Cache:
            return space_id == SaveDataSpaceId::User || space_id == SaveDataSpaceId::SdUser;
        default:
            UNREACHABLE();
            return false;
        }
    }

    static bool IsReconstructible(SaveDataType type, SaveDataSpaceId space_id) {
        switch (space_id) {
        case SaveDataSpaceId::System:
        case SaveDataSpaceId::User:
        case SaveDataSpaceId::ProperSystem:
        case SaveDataSpaceId::SafeMode:
            switch (type) {
            case SaveDataType::System:
            case SaveDataType::Account:
            case SaveDataType::Device:
                return false;
            case SaveDataType::Bcat:
            case SaveDataType::Temporary:
            case SaveDataType::Cache:
                return true;
            default:
                UNREACHABLE();
                return false;
            }
        case SaveDataSpaceId::SdSystem:
        case SaveDataSpaceId::Temporary:
        case SaveDataSpaceId::SdUser:
            return true;
        default:
            UNREACHABLE();
            return false;
        }
    }
};

} // namespace FileSys::FsSrv::Impl
