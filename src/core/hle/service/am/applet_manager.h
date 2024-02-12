// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <mutex>

#include "core/hle/service/am/applet.h"

namespace Core {
class System;
}

namespace Service::AM {

enum class LaunchType {
    FrontendInitiated,
    ApplicationInitiated,
};

struct FrontendAppletParameters {
    ProgramId program_id{};
    AppletId applet_id{};
    AppletType applet_type{};
    LaunchType launch_type{};
    s32 program_index{};
    s32 previous_program_index{-1};
};

class AppletManager {
public:
    explicit AppletManager(Core::System& system);
    ~AppletManager();

    void InsertApplet(std::shared_ptr<Applet> applet);
    void TerminateAndRemoveApplet(u64 aruid);

    void CreateAndInsertByFrontendAppletParameters(u64 aruid,
                                                   const FrontendAppletParameters& params);
    std::shared_ptr<Applet> GetByAppletResourceUserId(u64 aruid) const;

    void Reset();

    void RequestExit();
    void RequestResume();
    void OperationModeChanged();

private:
    Core::System& m_system;

    mutable std::mutex m_lock{};
    std::map<u64, std::shared_ptr<Applet>> m_applets{};

    // AudioController state goes here
};

} // namespace Service::AM
