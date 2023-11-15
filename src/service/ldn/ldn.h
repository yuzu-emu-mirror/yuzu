// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/result.h"
#include "kernel/k_event.h"
#include "service/ipc_helpers.h"
#include "service/kernel_helpers.h"
#include "service/sm/sm.h"

namespace Core {
class System;
}

namespace Service::LDN {

void LoopProcess(Core::System& system);

} // namespace Service::LDN
