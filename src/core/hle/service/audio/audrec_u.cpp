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
            {0, nullptr, "GetFinalOutputRecorderState"},
            {1, nullptr, "StartFinalOutputRecorder"},
            {2, nullptr, "StopFinalOutputRecorder"},
            {3, nullptr, "AppendFinalOutputRecorderBuffer"},
            {4, nullptr, "RegisterBufferEvent"},
            {5, nullptr, "GetReleasedFinalOutputRecorderBuffer"},
            {6, nullptr, "ContainsFinalOutputRecorderBuffer"},
            {8, nullptr, "AppendFinalOutputRecorderBufferAuto"},
            {9, nullptr, "GetReleasedFinalOutputRecorderBufferAuto"},
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
