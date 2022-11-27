// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#if defined(ARCHITECTURE_x86_64) || defined(ARCHITECTURE_arm64)
#include "core/arm/dynarmic/arm_exclusive_monitor.h"
#endif
#include "core/arm/exclusive_monitor.h"
#include "core/memory.h"

namespace Core {

ExclusiveMonitor::~ExclusiveMonitor() = default;

std::unique_ptr<Core::ExclusiveMonitor> MakeExclusiveMonitor(Memory::Memory& memory,
                                                             std::size_t num_cores) {
#if defined(ARCHITECTURE_x86_64) || defined(ARCHITECTURE_arm64)
    const bool no_locking =
        Settings::values.cpu_accuracy.GetValue() == Settings::CPUAccuracy::Auto ||
        (Settings::values.cpu_accuracy.GetValue() == Settings::CPUAccuracy::Unsafe &&
         Settings::values.cpuopt_unsafe_ignore_global_monitor);
    if (no_locking)
        return std::make_unique<Core::DynarmicExclusiveMonitorNoLocking>(memory, num_cores);
    return std::make_unique<Core::DynarmicExclusiveMonitor>(memory, num_cores);
#else
    // TODO(merry): Passthrough exclusive monitor
    return nullptr;
#endif
}

} // namespace Core
