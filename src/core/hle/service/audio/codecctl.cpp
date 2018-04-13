// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/audio/codecctl.h"

namespace Service {
namespace Audio {

CodecCtl::CodecCtl() : ServiceFramework("codecctl") {
    static const FunctionInfo functions[] = {
        {0x00000000, nullptr, "InitializeCodecController"},
        {0x00000001, nullptr, "FinalizeCodecController"},
        {0x00000002, nullptr, "SleepCodecController"},
        {0x00000003, nullptr, "WakeCodecController"},
        {0x00000004, nullptr, "SetCodecVolume"},
        {0x00000005, nullptr, "GetCodecVolumeMax"},
        {0x00000006, nullptr, "GetCodecVolumeMin"},
        {0x00000007, nullptr, "SetCodecActiveTarget"},
        {0x00000008, nullptr, "Unknown"},
        {0x00000009, nullptr, "BindCodecHeadphoneMicJackInterrupt"},
        {0x00000010, nullptr, "IsCodecHeadphoneMicJackInserted"},
        {0x00000011, nullptr, "ClearCodecHeadphoneMicJackInterrupt"},
        {0x00000012, nullptr, "IsCodecDeviceRequested"},
    };
    RegisterHandlers(functions);
}

} // namespace Audio
} // namespace Service
