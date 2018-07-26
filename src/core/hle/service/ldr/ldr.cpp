// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>

#include "core/hle/service/ldr/ldr.h"
#include "core/hle/service/service.h"

namespace Service::LDR {

class DebugMonitor final : public ServiceFramework<DebugMonitor> {
public:
    explicit DebugMonitor() : ServiceFramework{"ldr:dmnt"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "AddProcessToDebugLaunchQueue"},
            {1, nullptr, "ClearDebugLaunchQueue"},
            {2, nullptr, "GetNsoInfos"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class ProcessManager final : public ServiceFramework<ProcessManager> {
public:
    explicit ProcessManager() : ServiceFramework{"ldr:pm"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "CreateProcess"},
            {1, nullptr, "GetProgramInfo"},
            {2, nullptr, "RegisterTitle"},
            {3, nullptr, "UnregisterTitle"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class Shell final : public ServiceFramework<Shell> {
public:
    explicit Shell() : ServiceFramework{"ldr:shel"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "AddProcessToLaunchQueue"},
            {1, nullptr, "ClearLaunchQueue"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class RelocatableObject final : public ServiceFramework<RelocatableObject> {
public:
    explicit RelocatableObject() : ServiceFramework{"ldr:ro"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "LoadNro"},
            {1, nullptr, "UnloadNro"},
            {2, nullptr, "LoadNrr"},
            {3, nullptr, "UnloadNrr"},
            {4, nullptr, "Initialize"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void InstallInterfaces(SM::ServiceManager& sm) {
    std::make_shared<DebugMonitor>()->InstallAsService(sm);
    std::make_shared<ProcessManager>()->InstallAsService(sm);
    std::make_shared<Shell>()->InstallAsService(sm);
    std::make_shared<RelocatableObject>()->InstallAsService(sm);
}

} // namespace Service::LDR
