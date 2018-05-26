// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/am/applet_oe.h"
#include "core/hle/service/nvflinger/nvflinger.h"

namespace Service::AM {

class IApplicationProxy final : public ServiceFramework<IApplicationProxy> {
public:
    IApplicationProxy(std::shared_ptr<NVFlinger::NVFlinger> nvflinger)
        : ServiceFramework("IApplicationProxy"), nvflinger(std::move(nvflinger)) {
        static const FunctionInfo functions[] = {
            {0, &IApplicationProxy::GetCommonStateGetter, "GetCommonStateGetter"},
            {1, &IApplicationProxy::GetSelfController, "GetSelfController"},
            {2, &IApplicationProxy::GetWindowController, "GetWindowController"},
            {3, &IApplicationProxy::GetAudioController, "GetAudioController"},
            {4, &IApplicationProxy::GetDisplayController, "GetDisplayController"},
            {10, nullptr, "GetProcessWindingController"},
            {11, &IApplicationProxy::GetLibraryAppletCreator, "GetLibraryAppletCreator"},
            {20, &IApplicationProxy::GetApplicationFunctions, "GetApplicationFunctions"},
            {1000, &IApplicationProxy::GetDebugFunctions, "GetDebugFunctions"},
        };
        RegisterHandlers(functions);
    }

private:
    void GetAudioController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IAudioController>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetDisplayController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDisplayController>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetDebugFunctions(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDebugFunctions>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetWindowController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IWindowController>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetSelfController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ISelfController>(nvflinger);
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetCommonStateGetter(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ICommonStateGetter>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetLibraryAppletCreator(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ILibraryAppletCreator>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    void GetApplicationFunctions(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IApplicationFunctions>();
        NGLOG_DEBUG(Service_AM, "called");
    }

    std::shared_ptr<NVFlinger::NVFlinger> nvflinger;
};

void AppletOE::OpenApplicationProxy(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IApplicationProxy>(nvflinger);
    NGLOG_DEBUG(Service_AM, "called");
}

AppletOE::AppletOE(std::shared_ptr<NVFlinger::NVFlinger> nvflinger)
    : ServiceFramework("appletOE"), nvflinger(std::move(nvflinger)) {
    static const FunctionInfo functions[] = {
        {0, &AppletOE::OpenApplicationProxy, "OpenApplicationProxy"},
    };
    RegisterHandlers(functions);
}

} // namespace Service::AM
