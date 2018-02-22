// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/am/applet_ae.h"
#include "core/hle/service/am/applet_oe.h"
#include "core/hle/service/apm/apm.h"
#include "core/hle/service/nvflinger/nvflinger.h"

namespace Service {
namespace AM {

IWindowController::IWindowController() : ServiceFramework("IWindowController") {
    static const FunctionInfo functions[] = {
        {1, &IWindowController::GetAppletResourceUserId, "GetAppletResourceUserId"},
        {10, &IWindowController::AcquireForegroundRights, "AcquireForegroundRights"},
    };
    RegisterHandlers(functions);
}

void IWindowController::GetAppletResourceUserId(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u64>(0);
}

void IWindowController::AcquireForegroundRights(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_AM, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
}

IAudioController::IAudioController() : ServiceFramework("IAudioController") {}

IDisplayController::IDisplayController() : ServiceFramework("IDisplayController") {}

IDebugFunctions::IDebugFunctions() : ServiceFramework("IDebugFunctions") {}

ISelfController::ISelfController(std::shared_ptr<NVFlinger::NVFlinger> nvflinger)
    : ServiceFramework("ISelfController"), nvflinger(std::move(nvflinger)) {
    static const FunctionInfo functions[] = {
        {1, &ISelfController::LockExit, "LockExit"},
        {2, &ISelfController::UnlockExit, "UnlockExit"},
        {9, &ISelfController::GetLibraryAppletLaunchableEvent, "GetLibraryAppletLaunchableEvent"},
        {10, &ISelfController::SetScreenShotPermission, "SetScreenShotPermission"},
        {11, &ISelfController::SetOperationModeChangedNotification,
         "SetOperationModeChangedNotification"},
        {12, &ISelfController::SetPerformanceModeChangedNotification,
         "SetPerformanceModeChangedNotification"},
        {13, &ISelfController::SetFocusHandlingMode, "SetFocusHandlingMode"},
        {14, &ISelfController::SetRestartMessageEnabled, "SetRestartMessageEnabled"},
        {16, &ISelfController::SetOutOfFocusSuspendingEnabled, "SetOutOfFocusSuspendingEnabled"},
        {40, &ISelfController::CreateManagedDisplayLayer, "CreateManagedDisplayLayer"},
    };
    RegisterHandlers(functions);

    launchable_event =
        Kernel::Event::Create(Kernel::ResetType::OneShot, "ISelfController:LaunchableEvent");
}

void ISelfController::SetFocusHandlingMode(Kernel::HLERequestContext& ctx) {
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

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void ISelfController::SetRestartMessageEnabled(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void ISelfController::SetPerformanceModeChangedNotification(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    bool flag = rp.Pop<bool>();

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_AM, "(STUBBED) called flag=%u", static_cast<u32>(flag));
}

void ISelfController::SetScreenShotPermission(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void ISelfController::SetOperationModeChangedNotification(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    bool flag = rp.Pop<bool>();

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_AM, "(STUBBED) called flag=%u", static_cast<u32>(flag));
}

void ISelfController::SetOutOfFocusSuspendingEnabled(Kernel::HLERequestContext& ctx) {
    // Takes 3 input u8s with each field located immediately after the previous u8, these are
    // bool flags. No output.
    IPC::RequestParser rp{ctx};

    bool enabled = rp.Pop<bool>();

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_AM, "(STUBBED) called enabled=%u", static_cast<u32>(enabled));
}

void ISelfController::LockExit(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void ISelfController::UnlockExit(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void ISelfController::GetLibraryAppletLaunchableEvent(Kernel::HLERequestContext& ctx) {
    launchable_event->Signal();

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushCopyObjects(launchable_event);

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void ISelfController::CreateManagedDisplayLayer(Kernel::HLERequestContext& ctx) {
    // TODO(Subv): Find out how AM determines the display to use, for now just create the layer
    // in the Default display.
    u64 display_id = nvflinger->OpenDisplay("Default");
    u64 layer_id = nvflinger->CreateLayer(display_id);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(RESULT_SUCCESS);
    rb.Push(layer_id);

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

ICommonStateGetter::ICommonStateGetter() : ServiceFramework("ICommonStateGetter") {
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

void ICommonStateGetter::GetEventHandle(Kernel::HLERequestContext& ctx) {
    event->Signal();

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(RESULT_SUCCESS);
    rb.PushCopyObjects(event);

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void ICommonStateGetter::ReceiveMessage(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(15);

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void ICommonStateGetter::GetCurrentFocusState(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push(static_cast<u8>(FocusState::InFocus));

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void ICommonStateGetter::GetOperationMode(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push(static_cast<u8>(OperationMode::Handheld));

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void ICommonStateGetter::GetPerformanceMode(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push(static_cast<u32>(APM::PerformanceMode::Handheld));

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

class ILibraryAppletAccessor final : public ServiceFramework<ILibraryAppletAccessor> {
public:
    explicit ILibraryAppletAccessor() : ServiceFramework("ILibraryAppletAccessor") {
        static const FunctionInfo functions[] = {
            {0, &ILibraryAppletAccessor::GetAppletStateChangedEvent, "GetAppletStateChangedEvent"},
            {1, nullptr, "IsCompleted"},
            {10, nullptr, "Start"},
            {20, nullptr, "RequestExit"},
            {25, nullptr, "Terminate"},
            {30, nullptr, "GetResult"},
            {50, nullptr, "SetOutOfFocusApplicationSuspendingEnabled"},
            {100, nullptr, "PushInData"},
            {101, nullptr, "PopOutData"},
            {102, nullptr, "PushExtraStorage"},
            {103, nullptr, "PushInteractiveInData"},
            {104, nullptr, "PopInteractiveOutData"},
            {105, nullptr, "GetPopOutDataEvent"},
            {106, nullptr, "GetPopInteractiveOutDataEvent"},
            {120, nullptr, "NeedsToExitProcess"},
            {120, nullptr, "GetLibraryAppletInfo"},
            {150, nullptr, "RequestForAppletToGetForeground"},
            {160, nullptr, "GetIndirectLayerConsumerHandle"},
        };
        RegisterHandlers(functions);

        state_changed_event = Kernel::Event::Create(Kernel::ResetType::OneShot,
                                                    "ILibraryAppletAccessor:StateChangedEvent");
    }

private:
    void GetAppletStateChangedEvent(Kernel::HLERequestContext& ctx) {
        state_changed_event->Signal();

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(state_changed_event);

        LOG_WARNING(Service_AM, "(STUBBED) called");
    }

    Kernel::SharedPtr<Kernel::Event> state_changed_event;
};

ILibraryAppletCreator::ILibraryAppletCreator() : ServiceFramework("ILibraryAppletCreator") {
    static const FunctionInfo functions[] = {
        {0, &ILibraryAppletCreator::CreateLibraryApplet, "CreateLibraryApplet"},
        {1, nullptr, "TerminateAllLibraryApplets"},
        {2, nullptr, "AreAnyLibraryAppletsLeft"},
        {10, nullptr, "CreateStorage"},
        {11, nullptr, "CreateTransferMemoryStorage"},
        {12, nullptr, "CreateHandleStorage"},
    };
    RegisterHandlers(functions);
}

void ILibraryAppletCreator::CreateLibraryApplet(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};

    rb.Push(RESULT_SUCCESS);
    rb.PushIpcInterface<AM::ILibraryAppletAccessor>();

    LOG_DEBUG(Service_AM, "called");
}

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

        LOG_DEBUG(Service_AM, "called");
    }

    void Read(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        u64 offset = rp.Pop<u64>();

        const size_t size{ctx.GetWriteBufferSize()};

        ASSERT(offset + size <= buffer.size());

        ctx.WriteBuffer(buffer.data() + offset, size);

        IPC::ResponseBuilder rb{ctx, 2};

        rb.Push(RESULT_SUCCESS);

        LOG_DEBUG(Service_AM, "called");
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

        LOG_DEBUG(Service_AM, "called");
    }
};

IApplicationFunctions::IApplicationFunctions() : ServiceFramework("IApplicationFunctions") {
    static const FunctionInfo functions[] = {
        {1, &IApplicationFunctions::PopLaunchParameter, "PopLaunchParameter"},
        {20, &IApplicationFunctions::EnsureSaveData, "EnsureSaveData"},
        {21, &IApplicationFunctions::GetDesiredLanguage, "GetDesiredLanguage"},
        {22, &IApplicationFunctions::SetTerminateResult, "SetTerminateResult"},
        {66, &IApplicationFunctions::InitializeGamePlayRecording, "InitializeGamePlayRecording"},
        {67, &IApplicationFunctions::SetGamePlayRecordingState, "SetGamePlayRecordingState"},
        {40, &IApplicationFunctions::NotifyRunning, "NotifyRunning"},
    };
    RegisterHandlers(functions);
}

void IApplicationFunctions::PopLaunchParameter(Kernel::HLERequestContext& ctx) {
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

    LOG_DEBUG(Service_AM, "called");
}

void IApplicationFunctions::EnsureSaveData(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u64>(0);
}

void IApplicationFunctions::SetTerminateResult(Kernel::HLERequestContext& ctx) {
    // Takes an input u32 Result, no output.
    // For example, in some cases official apps use this with error 0x2A2 then uses svcBreak.

    IPC::RequestParser rp{ctx};
    u32 result = rp.Pop<u32>();

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_AM, "(STUBBED) called, result=0x%08X", result);
}

void IApplicationFunctions::GetDesiredLanguage(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u64>(SystemLanguage::English);
    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void IApplicationFunctions::InitializeGamePlayRecording(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);
    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void IApplicationFunctions::SetGamePlayRecordingState(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(RESULT_SUCCESS);

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void IApplicationFunctions::NotifyRunning(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u8>(0); // Unknown, seems to be ignored by official processes

    LOG_WARNING(Service_AM, "(STUBBED) called");
}

void InstallInterfaces(SM::ServiceManager& service_manager,
                       std::shared_ptr<NVFlinger::NVFlinger> nvflinger) {
    std::make_shared<AppletAE>(nvflinger)->InstallAsService(service_manager);
    std::make_shared<AppletOE>(nvflinger)->InstallAsService(service_manager);
}

} // namespace AM
} // namespace Service
