// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/service/audio/audrec_u.h"

namespace Service {
namespace Audio {

class IFinalOutputRecorder final : public ServiceFramework<IFinalOutputRecorder> {
public:
    IFinalOutputRecorder() : ServiceFramework("IFinalOutputRecorder") {
        static const FunctionInfo functions[] = {
            {0x0, nullptr, "GetFinalOutputRecorderState"},
            {0x1, nullptr, "StartFinalOutputRecorder"},
            {0x2, nullptr, "StopFinalOutputRecorder"},
            {0x3, nullptr, "AppendFinalOutputRecorderBuffer"},
            {0x4, nullptr, "RegisterBufferEvent"},
            {0x5, nullptr, "GetReleasedFinalOutputRecorderBuffer"},
            {0x6, nullptr, "ContainsFinalOutputRecorderBuffer"},
        };
        RegisterHandlers(functions);
    }
    ~IFinalOutputRecorder() = default;
};

AudRecU::AudRecU() : ServiceFramework("audrec:u") {
    static const FunctionInfo functions[] = {
        {0x00000000, nullptr, "OpenFinalOutputRecorder"},
    };
    RegisterHandlers(functions);
}

} // namespace Audio
} // namespace Service
