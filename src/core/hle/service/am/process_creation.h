// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <vector>

#include "common/common_types.h"

namespace Core {
class System;
}

namespace Service {
class Process;
}

namespace Service::AM {

std::unique_ptr<Process> CreateProcess(Core::System& system, u64 program_id,
                                       u8 minimum_key_generation, u8 maximum_key_generation);
std::unique_ptr<Process> CreateApplicationProcess(std::vector<u8>& out_control,
                                                  Core::System& system, u64 application_id,
                                                  u64 program_id);

} // namespace Service::AM
