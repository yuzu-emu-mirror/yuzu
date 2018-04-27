// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nfp/nfp.h"
#include "core/hle/service/nfp/nfp_user.h"

namespace Service::NFP {

Module::Interface::Interface(std::shared_ptr<Module> module, const char* name)
    : ServiceFramework(name), module(std::move(module)) {}

class IUser final : public ServiceFramework<IUser> {
public:
    IUser() : ServiceFramework("IUser") {
        static const FunctionInfo functions[] = {
            {0, &IUser::Initialize, "Initialize"},
            {1, nullptr, "Unknown1"},
            {2, nullptr, "Unknown2"},
            {3, nullptr, "Unknown3"},
            {4, nullptr, "Unknown4"},
            {5, nullptr, "Unknown5"},
            {6, nullptr, "Unknown6"},
            {7, nullptr, "Unknown7"},
            {8, nullptr, "Unknown8"},
            {9, nullptr, "Unknown9"},
            {10, nullptr, "Unknown10"},
            {11, nullptr, "Unknown11"},
            {12, nullptr, "Unknown12"},
            {13, nullptr, "Unknown13"},
            {14, nullptr, "Unknown14"},
            {15, nullptr, "Unknown15"},
            {16, nullptr, "Unknown16"},
            {17, nullptr, "Unknown17"},
            {18, nullptr, "Unknown18"},
            {19, nullptr, "Unknown19"},
            {20, nullptr, "Unknown20"},
            {21, nullptr, "Unknown21"},
            {22, nullptr, "Unknown22"},
            {23, nullptr, "Unknown23"},
            {24, nullptr, "Unknown24"},
        };
        RegisterHandlers(functions);
    }

private:
    void Initialize(Kernel::HLERequestContext& ctx) {
        NGLOG_WARNING(Service_NFP, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }
};

void Module::Interface::CreateUserInterface(Kernel::HLERequestContext& ctx) {
    NGLOG_DEBUG(Service_NFP, "called");
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IUser>();
}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    auto module = std::make_shared<Module>();
    std::make_shared<NFP_User>(module)->InstallAsService(service_manager);
}

} // namespace Service::NFP
