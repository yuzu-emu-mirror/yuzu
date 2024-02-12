// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/hle/service/am/applet.h"
#include "core/hle/service/am/applet_manager.h"

namespace Service::AM {

Applet::Applet(Core::System& system, std::unique_ptr<Process> process_, bool is_application)
    : context(system, "Applet"), lifecycle_manager(system, context, is_application),
      process(std::move(process_)), hid_registration(system, *process),
      gpu_error_detected_event(context), friend_invitation_storage_channel_event(context),
      notification_storage_channel_event(context), health_warning_disappeared_system_event(context),
      acquired_sleep_lock_event(context), pop_from_general_channel_event(context),
      library_applet_launchable_event(context), accumulated_suspended_tick_changed_event(context),
      sleep_lock_event(context), state_changed_event(context) {

    aruid = process->GetProcessId();
    program_id = process->GetProgramId();
}

Applet::~Applet() = default;

void Applet::UpdateSuspensionStateLocked(bool force_message) {
    // Remove any forced resumption.
    lifecycle_manager.RemoveForceResumeIfPossible();

    // Check if we're runnable.
    const bool is_runnable = lifecycle_manager.IsRunnable();
    const bool was_running = running;

    if (is_runnable != was_running) {
        if (is_runnable) {
            process->Suspend(false);
        } else {
            process->Suspend(true);
            lifecycle_manager.RequestResumeNotification();
        }

        running = is_runnable;
    }

    if (lifecycle_manager.GetForcedSuspend()) {
        // TODO: why is this allowed?
        return;
    }

    // Signal if the focus state was changed or the process state was changed.
    if (lifecycle_manager.UpdateRequestedFocusState() || is_runnable != was_running ||
        force_message) {
        lifecycle_manager.SignalSystemEventIfNeeded();
    }
}

void Applet::SetInteractibleLocked(bool interactible) {
    if (is_interactible == interactible) {
        return;
    }

    is_interactible = interactible;

    const bool new_state =
        window_visible && is_interactible && !lifecycle_manager.GetExitRequested();
    display_layer_manager.SetWindowVisibility(new_state);
    hid_registration.EnableAppletToGetInput(new_state);
}

void Applet::OnProcessTerminatedLocked() {
    is_completed = true;
    state_changed_event.Signal();
}

} // namespace Service::AM
