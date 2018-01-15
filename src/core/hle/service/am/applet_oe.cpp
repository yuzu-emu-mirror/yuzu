// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/service/am/applet_oe.h"

namespace Service {
namespace AM {

class IWindowController final : public ServiceFramework<IWindowController> {
public:
    IWindowController() : ServiceFramework("IWindowController") {
        static const FunctionInfo functions[] = {
            {1, &IWindowController::GetAppletResourceUserId, "GetAppletResourceUserId"},
            {10, &IWindowController::AcquireForegroundRights, "AcquireForegroundRights"},
        };
        RegisterHandlers(functions);
    }

private:
    void GetAppletResourceUserId(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service, "(STUBBED) called");
        IPC::RequestBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u64>(0);
    }

    void AcquireForegroundRights(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service, "(STUBBED) called");
        IPC::RequestBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
    }
};

class IAudioController final : public ServiceFramework<IAudioController> {
public:
    IAudioController() : ServiceFramework("IAudioController") {}
};

class IDisplayController final : public ServiceFramework<IDisplayController> {
public:
    IDisplayController() : ServiceFramework("IDisplayController") {}
};

class IDebugFunctions final : public ServiceFramework<IDebugFunctions> {
public:
    IDebugFunctions() : ServiceFramework("IDebugFunctions") {}
};

class ISelfController final : public ServiceFramework<ISelfController> {
public:
    ISelfController() : ServiceFramework("ISelfController") {}
};

class ICommonStateGetter final : public ServiceFramework<ICommonStateGetter> {
public:
    ICommonStateGetter() : ServiceFramework("ICommonStateGetter") {
        static const FunctionInfo functions[] = {
            {0, &ICommonStateGetter::GetEventHandle, "GetEventHandle"},
            {1, &ICommonStateGetter::ReceiveMessage, "ReceiveMessage"},
        };
        RegisterHandlers(functions);

        event = Kernel::Event::Create(Kernel::ResetType::OneShot, "ICommonStateGetter:Event");
    }

private:
    void GetEventHandle(Kernel::HLERequestContext& ctx) {
        event->Signal();

        IPC::RequestBuilder rb{ctx, 2, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(event);

        LOG_WARNING(Service, "(STUBBED) called");
    }

    void ReceiveMessage(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(15);

        LOG_WARNING(Service, "(STUBBED) called");
    }

    Kernel::SharedPtr<Kernel::Event> event;
};

class IApplicationFunctions final : public ServiceFramework<IApplicationFunctions> {
public:
    IApplicationFunctions() : ServiceFramework("IApplicationFunctions") {}
};

class ILibraryAppletCreator final : public ServiceFramework<ILibraryAppletCreator> {
public:
    ILibraryAppletCreator() : ServiceFramework("ILibraryAppletCreator") {}
};

class IApplicationProxy final : public ServiceFramework<IApplicationProxy> {
public:
    IApplicationProxy() : ServiceFramework("IApplicationProxy") {
        static const FunctionInfo functions[] = {
            {0, &IApplicationProxy::GetCommonStateGetter, "GetCommonStateGetter"},
            {1, &IApplicationProxy::GetSelfController, "GetSelfController"},
            {2, &IApplicationProxy::GetWindowController, "GetWindowController"},
            {3, &IApplicationProxy::GetAudioController, "GetAudioController"},
            {4, &IApplicationProxy::GetDisplayController, "GetDisplayController"},
            {11, &IApplicationProxy::GetLibraryAppletCreator, "GetLibraryAppletCreator"},
            {20, &IApplicationProxy::GetApplicationFunctions, "GetApplicationFunctions"},
            {1000, &IApplicationProxy::GetDebugFunctions, "GetDebugFunctions"},
        };
        RegisterHandlers(functions);
    }

private:
    void GetAudioController(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IAudioController>();
    }

    void GetDisplayController(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDisplayController>();
    }

    void GetDebugFunctions(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDebugFunctions>();
    }

    void GetWindowController(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IWindowController>();
    }

    void GetSelfController(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ISelfController>();
    }

    void GetCommonStateGetter(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ICommonStateGetter>();
    }

    void GetLibraryAppletCreator(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ILibraryAppletCreator>();
    }

    void GetApplicationFunctions(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IApplicationFunctions>();
    }
};

void AppletOE::OpenApplicationProxy(Kernel::HLERequestContext& ctx) {
    IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IApplicationProxy>();
}

AppletOE::AppletOE() : ServiceFramework("appletOE") {
    static const FunctionInfo functions[] = {
        {0x00000000, &AppletOE::OpenApplicationProxy, "OpenApplicationProxy"},
    };
    RegisterHandlers(functions);
}

} // namespace AM
} // namespace Service
