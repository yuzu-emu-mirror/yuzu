// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/service/am/applet_oe.h"
#include "core/hle/service/apm/apm.h"
#include "core/hle/service/nvflinger/nvflinger.h"

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
        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u64>(0);
    }

    void AcquireForegroundRights(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 2};
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
    ISelfController(std::shared_ptr<NVFlinger::NVFlinger> nvflinger)
        : ServiceFramework("ISelfController"), nvflinger(std::move(nvflinger)) {
        static const FunctionInfo functions[] = {
            {1, &ISelfController::LockExit, "LockExit"},
            {2, &ISelfController::UnlockExit, "UnlockExit"},
            {11, &ISelfController::SetOperationModeChangedNotification,
             "SetOperationModeChangedNotification"},
            {12, &ISelfController::SetPerformanceModeChangedNotification,
             "SetPerformanceModeChangedNotification"},
            {13, &ISelfController::SetFocusHandlingMode, "SetFocusHandlingMode"},
            {14, &ISelfController::SetRestartMessageEnabled, "SetRestartMessageEnabled"},
            {16, &ISelfController::SetOutOfFocusSuspendingEnabled,
             "SetOutOfFocusSuspendingEnabled"},
            {40, &ISelfController::CreateManagedDisplayLayer, "CreateManagedDisplayLayer"},
        };
        RegisterHandlers(functions);
    }

private:
    void SetFocusHandlingMode(Kernel::HLERequestContext& ctx) {
        // Takes 3 input u8s with each field located immediately after the previous u8, these are
        // bool flags. No output.

        IPC::RequestParser rp{ctx};

        struct FocusHandlingModeParams {
            u8 unknown0;
            u8 unknown1;
            u8 unknown2;
        };
        auto flags = rp.PopRaw<FocusHandlingModeParams>();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);

        LOG_WARNING(Service, "(STUBBED) called");
    }

    void SetRestartMessageEnabled(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);

        LOG_WARNING(Service, "(STUBBED) called");
    }

    void SetPerformanceModeChangedNotification(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        bool flag = rp.Pop<bool>();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);

        LOG_WARNING(Service, "(STUBBED) called flag=%u", static_cast<u32>(flag));
    }

    void SetOperationModeChangedNotification(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        bool flag = rp.Pop<bool>();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);

        LOG_WARNING(Service, "(STUBBED) called flag=%u", static_cast<u32>(flag));
    }

    void SetOutOfFocusSuspendingEnabled(Kernel::HLERequestContext& ctx) {
        // Takes 3 input u8s with each field located immediately after the previous u8, these are
        // bool flags. No output.
        IPC::RequestParser rp{ctx};

        bool enabled = rp.Pop<bool>();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);

        LOG_WARNING(Service, "(STUBBED) called enabled=%u", static_cast<u32>(enabled));
    }

    void LockExit(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);

        LOG_WARNING(Service, "(STUBBED) called");
    }

    void UnlockExit(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);

        LOG_WARNING(Service, "(STUBBED) called");
    }

    void CreateManagedDisplayLayer(Kernel::HLERequestContext& ctx) {
        // TODO(Subv): Find out how AM determines the display to use, for now just create the layer
        // in the Default display.
        u64 display_id = nvflinger->OpenDisplay("Default");
        u64 layer_id = nvflinger->CreateLayer(display_id);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push(layer_id);

        LOG_WARNING(Service, "(STUBBED) called");
    }

    std::shared_ptr<NVFlinger::NVFlinger> nvflinger;
};

class ICommonStateGetter final : public ServiceFramework<ICommonStateGetter> {
public:
    ICommonStateGetter() : ServiceFramework("ICommonStateGetter") {
        static const FunctionInfo functions[] = {
            {0, &ICommonStateGetter::GetEventHandle, "GetEventHandle"},
            {1, &ICommonStateGetter::ReceiveMessage, "ReceiveMessage"},
            {5, &ICommonStateGetter::GetOperationMode, "GetOperationMode"},
            {6, &ICommonStateGetter::GetPerformanceMode, "GetPerformanceMode"},
            {9, &ICommonStateGetter::GetCurrentFocusState, "GetCurrentFocusState"},
        };
        RegisterHandlers(functions);

        event = Kernel::Event::Create(Kernel::ResetType::OneShot, "ICommonStateGetter:Event");
    }

private:
    enum class FocusState : u8 {
        InFocus = 1,
        NotInFocus = 2,
    };

    enum class OperationMode : u8 {
        Handheld = 0,
        Docked = 1,
    };

    void GetEventHandle(Kernel::HLERequestContext& ctx) {
        event->Signal();

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(event);

        LOG_WARNING(Service, "(STUBBED) called");
    }

    void ReceiveMessage(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u32>(15);

        LOG_WARNING(Service, "(STUBBED) called");
    }

    void GetCurrentFocusState(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push(static_cast<u8>(FocusState::InFocus));

        LOG_WARNING(Service, "(STUBBED) called");
    }

    void GetOperationMode(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push(static_cast<u8>(OperationMode::Handheld));

        LOG_WARNING(Service, "(STUBBED) called");
    }

    void GetPerformanceMode(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push(static_cast<u32>(APM::PerformanceMode::Handheld));

        LOG_WARNING(Service, "(STUBBED) called");
    }

    Kernel::SharedPtr<Kernel::Event> event;
};

class IStorageAccessor final : public ServiceFramework<IStorageAccessor> {
public:
    explicit IStorageAccessor(std::vector<u8> buffer)
        : ServiceFramework("IStorageAccessor"), buffer(std::move(buffer)) {
        static const FunctionInfo functions[] = {
            {0, &IStorageAccessor::GetSize, "GetSize"},
            {11, &IStorageAccessor::Read, "Read"},
        };
        RegisterHandlers(functions);
    }

private:
    std::vector<u8> buffer;

    void GetSize(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 4};

        rb.Push(RESULT_SUCCESS);
        rb.Push(static_cast<u64>(buffer.size()));

        LOG_DEBUG(Service, "called");
    }

    void Read(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        u64 offset = rp.Pop<u64>();

        const auto& output_buffer = ctx.BufferDescriptorC()[0];

        ASSERT(offset + output_buffer.Size() <= buffer.size());

        Memory::WriteBlock(output_buffer.Address(), buffer.data() + offset, output_buffer.Size());

        IPC::ResponseBuilder rb{ctx, 2};

        rb.Push(RESULT_SUCCESS);

        LOG_DEBUG(Service, "called");
    }
};

class IStorage final : public ServiceFramework<IStorage> {
public:
    explicit IStorage(std::vector<u8> buffer)
        : ServiceFramework("IStorage"), buffer(std::move(buffer)) {
        static const FunctionInfo functions[] = {
            {0, &IStorage::Open, "Open"},
        };
        RegisterHandlers(functions);
    }

private:
    std::vector<u8> buffer;

    void Open(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};

        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<AM::IStorageAccessor>(buffer);

        LOG_DEBUG(Service, "called");
    }
};

class IApplicationFunctions final : public ServiceFramework<IApplicationFunctions> {
public:
    IApplicationFunctions() : ServiceFramework("IApplicationFunctions") {
        static const FunctionInfo functions[] = {
            {1, &IApplicationFunctions::PopLaunchParameter, "PopLaunchParameter"},
            {21, &IApplicationFunctions::GetDesiredLanguage, "GetDesiredLanguage"},
            {22, &IApplicationFunctions::SetTerminateResult, "SetTerminateResult"},
            {66, &IApplicationFunctions::InitializeGamePlayRecording,
             "InitializeGamePlayRecording"},
            {67, &IApplicationFunctions::SetGamePlayRecordingState, "SetGamePlayRecordingState"},
            {40, &IApplicationFunctions::NotifyRunning, "NotifyRunning"},
        };
        RegisterHandlers(functions);
    }

private:
    void PopLaunchParameter(Kernel::HLERequestContext& ctx) {
        constexpr u8 data[0x88] = {
            0xca, 0x97, 0x94, 0xc7, // Magic
            1,    0,    0,    0,    // IsAccountSelected (bool)
            1,    0,    0,    0,    // User Id (word 0)
            0,    0,    0,    0,    // User Id (word 1)
            0,    0,    0,    0,    // User Id (word 2)
            0,    0,    0,    0     // User Id (word 3)
        };

        std::vector<u8> buffer(data, data + sizeof(data));

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};

        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<AM::IStorage>(buffer);

        LOG_DEBUG(Service, "called");
    }

    void SetTerminateResult(Kernel::HLERequestContext& ctx) {
        // Takes an input u32 Result, no output.
        // For example, in some cases official apps use this with error 0x2A2 then uses svcBreak.

        IPC::RequestParser rp{ctx};
        u32 result = rp.Pop<u32>();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);

        LOG_WARNING(Service, "(STUBBED) called, result=0x%08X", result);
    }

    void GetDesiredLanguage(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u64>(SystemLanguage::English);
        LOG_WARNING(Service, "(STUBBED) called");
    }

    void InitializeGamePlayRecording(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service, "(STUBBED) called");
    }

    void SetGamePlayRecordingState(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);

        LOG_WARNING(Service, "(STUBBED) called");
    }

    void NotifyRunning(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(RESULT_SUCCESS);
        rb.Push<u8>(0); // Unknown, seems to be ignored by official processes

        LOG_WARNING(Service, "(STUBBED) called");
    }
};

class ILibraryAppletCreator final : public ServiceFramework<ILibraryAppletCreator> {
public:
    ILibraryAppletCreator() : ServiceFramework("ILibraryAppletCreator") {}
};

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
        LOG_DEBUG(Service, "called");
    }

    void GetDisplayController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDisplayController>();
        LOG_DEBUG(Service, "called");
    }

    void GetDebugFunctions(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IDebugFunctions>();
        LOG_DEBUG(Service, "called");
    }

    void GetWindowController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IWindowController>();
        LOG_DEBUG(Service, "called");
    }

    void GetSelfController(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ISelfController>(nvflinger);
        LOG_DEBUG(Service, "called");
    }

    void GetCommonStateGetter(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ICommonStateGetter>();
        LOG_DEBUG(Service, "called");
    }

    void GetLibraryAppletCreator(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<ILibraryAppletCreator>();
        LOG_DEBUG(Service, "called");
    }

    void GetApplicationFunctions(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IApplicationFunctions>();
        LOG_DEBUG(Service, "called");
    }

    std::shared_ptr<NVFlinger::NVFlinger> nvflinger;
};

void AppletOE::OpenApplicationProxy(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<IApplicationProxy>(nvflinger);
    LOG_DEBUG(Service, "called");
}

AppletOE::AppletOE(std::shared_ptr<NVFlinger::NVFlinger> nvflinger)
    : ServiceFramework("appletOE"), nvflinger(std::move(nvflinger)) {
    static const FunctionInfo functions[] = {
        {0x00000000, &AppletOE::OpenApplicationProxy, "OpenApplicationProxy"},
    };
    RegisterHandlers(functions);
}

} // namespace AM
} // namespace Service
