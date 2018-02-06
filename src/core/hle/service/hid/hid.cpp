// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <atomic>
#include "common/logging/log.h"
#include "core/core_timing.h"
#include "core/frontend/input.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/client_port.h"
#include "core/hle/kernel/client_session.h"
#include "core/hle/kernel/shared_memory.h"
#include "core/hle/service/hid/hid.h"
#include "core/hle/service/service.h"

namespace Service {
namespace HID {

// Updating period for each HID device.
// TODO(shinyquagsire23): These need better values.
constexpr u64 pad_update_ticks = BASE_CLOCK_RATE / 10000;
constexpr u64 accelerometer_update_ticks = BASE_CLOCK_RATE / 10000;
constexpr u64 gyroscope_update_ticks = BASE_CLOCK_RATE / 10000;

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

        // Register update callbacks
        pad_update_event = CoreTiming::RegisterEvent(
            "HID::UpdatePadCallback",
            [this](u64 userdata, int cycles_late) { UpdatePadCallback(userdata, cycles_late); });

        // TODO(shinyquagsire23): Other update callbacks? (accel, gyro?)

        CoreTiming::ScheduleEvent(pad_update_ticks, pad_update_event);
    }

private:
    void GetSharedMemoryHandle(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(shared_mem);
        LOG_DEBUG(Service_HID, "called");
    }

    void LoadInputDevices() {
        std::transform(Settings::values.buttons.begin() + Settings::NativeButton::BUTTON_HID_BEGIN,
                       Settings::values.buttons.begin() + Settings::NativeButton::BUTTON_HID_END,
                       buttons.begin(), Input::CreateDevice<Input::ButtonDevice>);
        // TODO(shinyquagsire23): sticks, gyro, touch, mouse, keyboard
    }

    void UpdatePadCallback(u64 userdata, int cycles_late) {
        SharedMemory* mem = reinterpret_cast<SharedMemory*>(shared_mem->GetPointer());

        if (is_device_reload_pending.exchange(false))
            LoadInputDevices();

        // Set up controllers as neon red+blue Joy-Con attached to console
        ControllerHeader& controller_header = mem->controllers[Controller_Handheld].header;
        controller_header.type = ControllerType_Handheld | ControllerType_JoyconPair;
        controller_header.single_colors_descriptor = ColorDesc_ColorsNonexistent;
        controller_header.right_color_body = JOYCON_BODY_NEON_RED;
        controller_header.right_color_buttons = JOYCON_BUTTONS_NEON_RED;
        controller_header.left_color_body = JOYCON_BODY_NEON_BLUE;
        controller_header.left_color_buttons = JOYCON_BUTTONS_NEON_BLUE;

        for (int layoutIdx = 0; layoutIdx < HID_NUM_LAYOUTS; layoutIdx++) {
            ControllerLayout& layout = mem->controllers[Controller_Handheld].layouts[layoutIdx];
            layout.header.num_entries = HID_NUM_ENTRIES;
            layout.header.max_entry_index = HID_NUM_ENTRIES - 1;

            // HID shared memory stores the state of the past 17 samples in a circlular buffer,
            // each with a timestamp in number of samples since boot.
            layout.header.timestamp_ticks = CoreTiming::GetTicks();
            layout.header.latest_entry = (layout.header.latest_entry + 1) % HID_NUM_ENTRIES;

            ControllerInputEntry& entry = layout.entries[layout.header.latest_entry];
            entry.connection_state = ConnectionState_Connected | ConnectionState_Wired;
            entry.timestamp++;
            entry.timestamp_2++; // TODO(shinyquagsire23): Is this always identical to timestamp?

            // TODO(shinyquagsire23): Set up some LUTs for each layout mapping in the future?
            // For now everything is just the default handheld layout, but split Joy-Con will
            // rotate the face buttons and directions for certain layouts.
            ControllerPadState& state = entry.buttons;
            using namespace Settings::NativeButton;
            state.a.Assign(buttons[A - BUTTON_HID_BEGIN]->GetStatus());
            state.b.Assign(buttons[B - BUTTON_HID_BEGIN]->GetStatus());
            state.x.Assign(buttons[X - BUTTON_HID_BEGIN]->GetStatus());
            state.y.Assign(buttons[Y - BUTTON_HID_BEGIN]->GetStatus());
            state.lstick.Assign(buttons[LStick - BUTTON_HID_BEGIN]->GetStatus());
            state.rstick.Assign(buttons[RStick - BUTTON_HID_BEGIN]->GetStatus());
            state.l.Assign(buttons[L - BUTTON_HID_BEGIN]->GetStatus());
            state.r.Assign(buttons[R - BUTTON_HID_BEGIN]->GetStatus());
            state.zl.Assign(buttons[ZL - BUTTON_HID_BEGIN]->GetStatus());
            state.zr.Assign(buttons[ZR - BUTTON_HID_BEGIN]->GetStatus());
            state.plus.Assign(buttons[Plus - BUTTON_HID_BEGIN]->GetStatus());
            state.minus.Assign(buttons[Minus - BUTTON_HID_BEGIN]->GetStatus());

            state.dleft.Assign(buttons[DLeft - BUTTON_HID_BEGIN]->GetStatus());
            state.dup.Assign(buttons[DUp - BUTTON_HID_BEGIN]->GetStatus());
            state.dright.Assign(buttons[DRight - BUTTON_HID_BEGIN]->GetStatus());
            state.ddown.Assign(buttons[DDown - BUTTON_HID_BEGIN]->GetStatus());

            state.lstick_left.Assign(buttons[LStick_Left - BUTTON_HID_BEGIN]->GetStatus());
            state.lstick_up.Assign(buttons[LStick_Up - BUTTON_HID_BEGIN]->GetStatus());
            state.lstick_right.Assign(buttons[LStick_Right - BUTTON_HID_BEGIN]->GetStatus());
            state.lstick_down.Assign(buttons[LStick_Down - BUTTON_HID_BEGIN]->GetStatus());

            state.rstick_left.Assign(buttons[RStick_Left - BUTTON_HID_BEGIN]->GetStatus());
            state.rstick_up.Assign(buttons[RStick_Up - BUTTON_HID_BEGIN]->GetStatus());
            state.rstick_right.Assign(buttons[RStick_Right - BUTTON_HID_BEGIN]->GetStatus());
            state.rstick_down.Assign(buttons[RStick_Down - BUTTON_HID_BEGIN]->GetStatus());

            state.sl.Assign(buttons[SL - BUTTON_HID_BEGIN]->GetStatus());
            state.sr.Assign(buttons[SR - BUTTON_HID_BEGIN]->GetStatus());

            // TODO(shinyquagsire23): Analog stick vals

            // TODO(shinyquagsire23): Update pad info proper, (circular buffers, timestamps,
            // layouts)
        }

        // TODO(shinyquagsire23): Update touch info

        // TODO(shinyquagsire23): Signal events

        // Reschedule recurrent event
        CoreTiming::ScheduleEvent(pad_update_ticks - cycles_late, pad_update_event);
    }

    // Handle to shared memory region designated to HID service
    Kernel::SharedPtr<Kernel::SharedMemory> shared_mem;

    // CoreTiming update events
    CoreTiming::EventType* pad_update_event;

    // Stored input state info
    std::atomic<bool> is_device_reload_pending{true};
    std::array<std::unique_ptr<Input::ButtonDevice>, Settings::NativeButton::NUM_BUTTONS_HID>
        buttons;
};

class IActiveVibrationDeviceList final : public ServiceFramework<IActiveVibrationDeviceList> {
public:
    IActiveVibrationDeviceList() : ServiceFramework("IActiveVibrationDeviceList") {
        static const FunctionInfo functions[] = {
            {0, &IActiveVibrationDeviceList::ActivateVibrationDevice, "ActivateVibrationDevice"},
        };
        RegisterHandlers(functions);
    }

private:
    void ActivateVibrationDevice(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_HID, "(STUBBED) called");
    }
};

class Hid final : public ServiceFramework<Hid> {
public:
    Hid() : ServiceFramework("hid") {
        static const FunctionInfo functions[] = {
            {0, &Hid::CreateAppletResource, "CreateAppletResource"},
            {1, &Hid::ActivateDebugPad, "ActivateDebugPad"},
            {11, &Hid::ActivateTouchScreen, "ActivateTouchScreen"},
            {66, &Hid::StartSixAxisSensor, "StartSixAxisSensor"},
            {100, &Hid::SetSupportedNpadStyleSet, "SetSupportedNpadStyleSet"},
            {102, &Hid::SetSupportedNpadIdType, "SetSupportedNpadIdType"},
            {103, &Hid::ActivateNpad, "ActivateNpad"},
            {120, &Hid::SetNpadJoyHoldType, "SetNpadJoyHoldType"},
            {124, nullptr, "SetNpadJoyAssignmentModeDual"},
            {203, &Hid::CreateActiveVibrationDeviceList, "CreateActiveVibrationDeviceList"},
        };
        RegisterHandlers(functions);
    }
    ~Hid() = default;

private:
    std::shared_ptr<IAppletResource> applet_resource;

    void CreateAppletResource(Kernel::HLERequestContext& ctx) {
        if (applet_resource == nullptr) {
            applet_resource = std::make_shared<IAppletResource>();
        }

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IAppletResource>(applet_resource);
        LOG_DEBUG(Service_HID, "called");
    }

    void ActivateDebugPad(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_HID, "(STUBBED) called");
    }

    void ActivateTouchScreen(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_HID, "(STUBBED) called");
    }

    void StartSixAxisSensor(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_HID, "(STUBBED) called");
    }

    void SetSupportedNpadStyleSet(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_HID, "(STUBBED) called");
    }

    void SetSupportedNpadIdType(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_HID, "(STUBBED) called");
    }

    void ActivateNpad(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_HID, "(STUBBED) called");
    }

    void SetNpadJoyHoldType(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_HID, "(STUBBED) called");
    }

    void CreateActiveVibrationDeviceList(Kernel::HLERequestContext& ctx) {
        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushIpcInterface<IActiveVibrationDeviceList>();
        LOG_DEBUG(Service_HID, "called");
    }
};

void ReloadInputDevices() {}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<Hid>()->InstallAsService(service_manager);
}

} // namespace HID
} // namespace Service
