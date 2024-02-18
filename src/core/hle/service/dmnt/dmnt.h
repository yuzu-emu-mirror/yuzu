// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace Core {
class System;
};

namespace Service::DMNT {

void LoopProcess(Core::System& system);

} // namespace Service::DMNT
