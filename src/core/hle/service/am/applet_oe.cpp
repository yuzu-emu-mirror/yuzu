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
    ISelfController() : ServiceFramework("ISelfController") {
        static const FunctionInfo functions[] = {
            {13, &ISelfController::SetFocusHandlingMode, "SetFocusHandlingMode"},
        };
        RegisterHandlers(functions);
    }

private:
    void SetFocusHandlingMode(Kernel::HLERequestContext& ctx) {
        // Takes 3 input u8s with each field located immediately after the previous u8, these are
        // bool flags. No output.

        IPC::RequestBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);

        LOG_WARNING(Service, "(STUBBED) called");
    }
};

class ICommonStateGetter final : public ServiceFramework<ICommonStateGetter> {
public:
    ICommonStateGetter() : ServiceFramework("ICommonStateGetter") {
        static const FunctionInfo functions[] = {
            {0, &ICommonStateGetter::GetEventHandle, "GetEventHandle"},
            {1, &ICommonStateGetter::ReceiveMessage, "ReceiveMessage"},
            {9, &ICommonStateGetter::GetCurrentFocusState, "GetCurrentFocusState"},
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

    void GetCurrentFocusState(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(1); // 1: In focus, 2/3: Out of focus(running in "background")

        LOG_WARNING(Service, "(STUBBED) called");
    }

    Kernel::SharedPtr<Kernel::Event> event;
};

class IApplicationFunctions final : public ServiceFramework<IApplicationFunctions> {
public:
    IApplicationFunctions() : ServiceFramework("IApplicationFunctions") {
        static const FunctionInfo functions[] = {
            {22, &IApplicationFunctions::SetTerminateResult, "SetTerminateResult"},
        };
        RegisterHandlers(functions);
    }

private:
    void SetTerminateResult(Kernel::HLERequestContext& ctx) {
        // Takes an input u32 Result, no output.
        // For example, in some cases official apps use this with error 0x2A2 then uses svcBreak.

        IPC::RequestParser rp{ctx};
        u32 result = rp.Pop<u32>();

        IPC::RequestBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);

        LOG_WARNING(Service, "(STUBBED) called, result=0x%08X", result);
    }
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
        LOG_DEBUG(Service, "called");
    }

    void GetDisplayController(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDisplayController>();
        LOG_DEBUG(Service, "called");
    }

    void GetDebugFunctions(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDebugFunctions>();
        LOG_DEBUG(Service, "called");
    }

    void GetWindowController(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IWindowController>();
        LOG_DEBUG(Service, "called");
    }

    void GetSelfController(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ISelfController>();
        LOG_DEBUG(Service, "called");
    }

    void GetCommonStateGetter(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ICommonStateGetter>();
        LOG_DEBUG(Service, "called");
    }

    void GetLibraryAppletCreator(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ILibraryAppletCreator>();
        LOG_DEBUG(Service, "called");
    }

    void GetApplicationFunctions(Kernel::HLERequestContext& ctx) {
        IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IApplicationFunctions>();
        LOG_DEBUG(Service, "called");
    }
};

void AppletOE::OpenApplicationProxy(Kernel::HLERequestContext& ctx) {
    IPC::RequestBuilder rb{ctx, 2, 0, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IApplicationProxy>();
    LOG_DEBUG(Service, "called");
}

AppletOE::AppletOE() : ServiceFramework("appletOE") {
    static const FunctionInfo functions[] = {
        {0x00000000, &AppletOE::OpenApplicationProxy, "OpenApplicationProxy"},
    };
    RegisterHandlers(functions);
}

} // namespace AM
} // namespace Service
