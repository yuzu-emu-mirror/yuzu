// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/kernel/shared_memory.h"
#include "core/hle/service/hid/hid.h"
#include "core/hle/service/service.h"

namespace Service {
namespace HID {

class IAppletResource final : public ServiceFramework<IAppletResource> {
public:
    IAppletResource() : ServiceFramework("IAppletResource") {
        static const FunctionInfo functions[] = {
            {0, &IAppletResource::GetSharedMemoryHandle, "GetSharedMemoryHandle"},
        };
        RegisterHandlers(functions);

        shared_mem = Kernel::SharedMemory::Create(
            nullptr, 0x40000, Kernel::MemoryPermission::ReadWrite, Kernel::MemoryPermission::Read,
            0, Kernel::MemoryRegion::BASE, "HID:SharedMemory");
    }

private:
    void GetSharedMemoryHandle(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(shared_mem);
        LOG_DEBUG(Service, "called");
    }

    // Handle to shared memory region designated to HID service
    Kernel::SharedPtr<Kernel::SharedMemory> shared_mem;
};

class Hid final : public ServiceFramework<Hid> {
public:
    Hid() : ServiceFramework("hid") {
        static const FunctionInfo functions[] = {
            {0x00000000, &Hid::CreateAppletResource, "CreateAppletResource"},
        };
        RegisterHandlers(functions);
    }
    ~Hid() = default;

private:
    void CreateAppletResource(Kernel::HLERequestContext& ctx) {
        auto client_port = std::make_shared<IAppletResource>()->CreatePort();
        auto session = client_port->Connect();
        if (session.Succeeded()) {
            LOG_DEBUG(Service, "called, initialized IAppletResource -> session=%u",
                      (*session)->GetObjectId());
            IPC::RequestBuilder rb{ctx, 2, 0, 1};
            rb.Push(RESULT_SUCCESS);
            rb.PushMoveObjects(std::move(session).Unwrap());
            registered_loggers.emplace_back(std::move(client_port));
        } else {
            UNIMPLEMENTED();
        }
    }

    std::vector<Kernel::SharedPtr<Kernel::ClientPort>> registered_loggers;
};

void ReloadInputDevices() {}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<Hid>()->InstallAsService(service_manager);
}

} // namespace HID
} // namespace Service
