// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <unordered_map>
#include "core/hle/kernel/kernel.h"
#include "core/hle/result.h"
#include "core/hle/service/service.h"

namespace Kernel {
class ClientPort;
class ClientSession;
class ServerPort;
class SessionRequestHandler;
} // namespace Kernel

namespace Service {
namespace SM {

/// Interface to "sm:" service
class SM final : public ServiceFramework<SM> {
public:
    SM(std::shared_ptr<ServiceManager> service_manager);
    ~SM() = default;

private:
    void Initialize(Kernel::HLERequestContext& ctx);
    void GetService(Kernel::HLERequestContext& ctx);

    std::shared_ptr<ServiceManager> service_manager;
};

class Controller;

constexpr ResultCode ERR_SERVICE_NOT_REGISTERED(-1);
constexpr ResultCode ERR_MAX_CONNECTIONS_REACHED(-1);
constexpr ResultCode ERR_INVALID_NAME_SIZE(-1);
constexpr ResultCode ERR_NAME_CONTAINS_NUL(-1);
constexpr ResultCode ERR_ALREADY_REGISTERED(-1);

class ServiceManager {
public:
    static void InstallInterfaces(std::shared_ptr<ServiceManager> self);

    ResultVal<Kernel::SharedPtr<Kernel::ServerPort>> RegisterService(std::string name,
                                                                     unsigned int max_sessions);
    ResultVal<Kernel::SharedPtr<Kernel::ClientPort>> GetServicePort(const std::string& name);
    ResultVal<Kernel::SharedPtr<Kernel::ClientSession>> ConnectToService(const std::string& name);

    void InvokeControlRequest(Kernel::HLERequestContext& context);

private:
    std::weak_ptr<SM> sm_interface;
    std::unique_ptr<Controller> controller_interface;

    /// Map of registered services, retrieved using GetServicePort or ConnectToService.
    std::unordered_map<std::string, Kernel::SharedPtr<Kernel::ClientPort>> registered_services;
};

extern std::shared_ptr<ServiceManager> g_service_manager;

} // namespace SM
} // namespace Service
