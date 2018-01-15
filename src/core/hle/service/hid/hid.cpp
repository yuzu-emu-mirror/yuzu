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
constexpr u64 pad_update_ticks = BASE_CLOCK_RATE / 234;
constexpr u64 accelerometer_update_ticks = BASE_CLOCK_RATE / 104;
constexpr u64 gyroscope_update_ticks = BASE_CLOCK_RATE / 101;

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
        IPC::RequestBuilder rb{ctx, 2, 1};
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(shared_mem);
        LOG_DEBUG(Service, "called");
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

        // TODO(shinyquagsire23): This is a hack!
        ControllerPadState& state =
            mem->controllers[Controller_Handheld].layouts[Layout_Default].entries[0].buttons;
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

        // TODO(shinyquagsire23): Update pad info proper, (circular buffers, timestamps, layouts)

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
        } else {
            UNIMPLEMENTED();
        }
    }
};

void ReloadInputDevices() {}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<Hid>()->InstallAsService(service_manager);
}

} // namespace HID
} // namespace Service
