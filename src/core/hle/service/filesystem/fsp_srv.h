// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "core/hle/kernel/writable_event.h"
#include "core/hle/service/service.h"

namespace Core {
class Reporter;
}

namespace FileSys {
class FileSystemBackend;
}

namespace Service::FileSystem {

enum class AccessLogVersion : u32 {
    V7_0_0 = 2,

    Latest = V7_0_0,
};

enum class LogMode : u32 {
    Off,
    Log,
    RedirectToSdCard,
    LogToSdCard = Log | RedirectToSdCard,
};

class FSP_SRV final : public ServiceFramework<FSP_SRV> {
public:
    explicit FSP_SRV(Core::System& system);
    ~FSP_SRV() override;

private:
    void SetCurrentProcess(Kernel::HLERequestContext& ctx);
    void OpenFileSystemWithPatch(Kernel::HLERequestContext& ctx);
    void OpenBisFileSystem(Kernel::HLERequestContext& ctx);
    void OpenBisStorage(Kernel::HLERequestContext& ctx);
    void InvalidateBisCache(Kernel::HLERequestContext& ctx);
    void OpenSdCardFileSystem(Kernel::HLERequestContext& ctx);
    void CreateSaveDataFileSystem(Kernel::HLERequestContext& ctx);
    void CreateSaveDataFileSystemBySystemSaveDataId(Kernel::HLERequestContext& ctx);
    void OpenSaveDataFileSystem(Kernel::HLERequestContext& ctx);
    void OpenSaveDataFileSystemBySystemSaveDataId(Kernel::HLERequestContext& ctx);
    void OpenReadOnlySaveDataFileSystem(Kernel::HLERequestContext& ctx);
    void OpenSaveDataInfoReader(Kernel::HLERequestContext& ctx);
    void OpenSaveDataInfoReaderBySaveDataSpaceId(Kernel::HLERequestContext& ctx);
    void OpenImageDirectoryFileSystem(Kernel::HLERequestContext& ctx);
    void OpenContentStorageFileSystem(Kernel::HLERequestContext& ctx);
    void SetGlobalAccessLogMode(Kernel::HLERequestContext& ctx);
    void GetGlobalAccessLogMode(Kernel::HLERequestContext& ctx);
    void OpenDataStorageByCurrentProcess(Kernel::HLERequestContext& ctx);
    void OpenDataStorageByDataId(Kernel::HLERequestContext& ctx);
    void OpenPatchDataStorageByCurrentProcess(Kernel::HLERequestContext& ctx);
    void OutputAccessLogToSdCard(Kernel::HLERequestContext& ctx);
    void GetAccessLogVersionInfo(Kernel::HLERequestContext& ctx);
    void OpenSdCardDetectionEventNotifier(Kernel::HLERequestContext& ctx);
    void OpenGameCardDetectionEventNotifier(Kernel::HLERequestContext& ctx);
    void SetSdCardEncryptionSeed(Kernel::HLERequestContext& ctx);

    Core::System& system;
    FileSystemController& fsc;

    FileSys::VirtualFile romfs;
    u64 current_process_id = 0;
    u32 access_log_program_index = 0;
    LogMode log_mode = LogMode::LogToSdCard;

    Kernel::EventPair sd_card_detection_event;
    Kernel::EventPair game_card_detection_event;
};

} // namespace Service::FileSystem
