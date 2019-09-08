// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/bcat/bcat.h"
#include "core/hle/service/bcat/module.h"

namespace Service::BCAT {

class IDeliveryCacheProgressService final : public ServiceFramework<IDeliveryCacheProgressService> {
public:
    IDeliveryCacheProgressService() : ServiceFramework("IDeliveryCacheProgressService") {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetEvent"},
            {1, nullptr, "GetImpl"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IBcatService final : public ServiceFramework<IBcatService> {
public:
    IBcatService() : ServiceFramework("IBcatService") {
        // clang-format off
        static const FunctionInfo functions[] = {
            {10100, &IBcatService::RequestSyncDeliveryCache, "RequestSyncDeliveryCache"},
            {10101, nullptr, "RequestSyncDeliveryCacheWithDirectoryName"},
            {10200, nullptr, "CancelSyncDeliveryCacheRequest"},
            {20100, nullptr, "RequestSyncDeliveryCacheWithApplicationId"},
            {20101, nullptr, "RequestSyncDeliveryCacheWithApplicationIdAndDirectoryName"},
            {30100, nullptr, "SetPassphrase"},
            {30200, nullptr, "RegisterBackgroundDeliveryTask"},
            {30201, nullptr, "UnregisterBackgroundDeliveryTask"},
            {30202, nullptr, "BlockDeliveryTask"},
            {30203, nullptr, "UnblockDeliveryTask"},
            {90100, nullptr, "EnumerateBackgroundDeliveryTask"},
            {90200, nullptr, "GetDeliveryList"},
            {90201, nullptr, "ClearDeliveryCacheStorage"},
            {90300, nullptr, "GetPushNotificationLog"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void RequestSyncDeliveryCache(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_BCAT, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDeliveryCacheProgressService>();
    }
};

class IDeliveryCacheFileService final : public ServiceFramework<IDeliveryCacheFileService> {
public:
    IDeliveryCacheFileService() : ServiceFramework("IDeliveryCacheFileService") {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Open"},
            {1, nullptr, "Read"},
            {2, nullptr, "GetSize"},
            {4, nullptr, "GetDigest"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IDeliveryCacheDirectoryService final
    : public ServiceFramework<IDeliveryCacheDirectoryService> {
public:
    IDeliveryCacheDirectoryService() : ServiceFramework("IDeliveryCacheDirectoryService") {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Open"},
            {1, nullptr, "Read"},
            {2, nullptr, "GetCount"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IDeliveryCacheStorageService final : public ServiceFramework<IDeliveryCacheStorageService> {
public:
    IDeliveryCacheStorageService() : ServiceFramework("IDeliveryCacheStorageService") {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IDeliveryCacheStorageService::CreateFileService, "CreateFileService"},
            {1, &IDeliveryCacheStorageService::CreateDirectoryService, "CreateDirectoryService"},
            {10, &IDeliveryCacheStorageService::EnumerateDeliveryCacheDirectory, "EnumerateDeliveryCacheDirectory"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void CreateFileService(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_BCAT, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDeliveryCacheFileService>();
    }

    void CreateDirectoryService(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_BCAT, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDeliveryCacheDirectoryService>();
    }

    void EnumerateDeliveryCacheDirectory(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_BCAT, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(0);
    }
};

void Module::Interface::CreateBcatService(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_BCAT, "called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IBcatService>();
}

void Module::Interface::CreateDeliveryCacheStorageService(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_BCAT, "called");

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IDeliveryCacheStorageService>();
}

Module::Interface::Interface(std::shared_ptr<Module> module, const char* name)
    : ServiceFramework(name), module(std::move(module)) {}

Module::Interface::~Interface() = default;

void InstallInterfaces(SM::ServiceManager& service_manager) {
    auto module = std::make_shared<Module>();
    std::make_shared<BCAT>(module, "bcat:a")->InstallAsService(service_manager);
    std::make_shared<BCAT>(module, "bcat:m")->InstallAsService(service_manager);
    std::make_shared<BCAT>(module, "bcat:u")->InstallAsService(service_manager);
    std::make_shared<BCAT>(module, "bcat:s")->InstallAsService(service_manager);
}

} // namespace Service::BCAT
