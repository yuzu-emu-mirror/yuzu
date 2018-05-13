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
            {1, nullptr, "Finalize"},
            {2, nullptr, "ListDevices"},
            {3, nullptr, "StartDetection"},
            {4, nullptr, "StopDetection"},
            {5, nullptr, "Mount"},
            {6, nullptr, "Unmount"},
            {7, nullptr, "OpenApplicationArea"},
            {8, nullptr, "GetApplicationArea"},
            {9, nullptr, "SetApplicationArea"},
            {10, nullptr, "Flush"},
            {11, nullptr, "Restore"},
            {12, nullptr, "CreateApplicationArea"},
            {13, nullptr, "GetTagInfo"},
            {14, nullptr, "GetRegisterInfo"},
            {15, nullptr, "GetCommonInfo"},
            {16, nullptr, "GetModelInfo"},
            {17, nullptr, "AttachActivateEvent"},
            {18, nullptr, "AttachDeactivateEvent"},
            {19, nullptr, "GetState"},
            {20, nullptr, "GetDeviceState"},
            {21, nullptr, "GetNpadId"},
            {22, nullptr, "GetApplicationArea2"},
            {23, nullptr, "AttachAvailabilityChangeEvent"},
            {24, nullptr, "RecreateApplicationArea"},
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
