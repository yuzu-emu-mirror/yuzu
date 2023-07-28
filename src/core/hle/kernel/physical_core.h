// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>

#include "core/arm/arm_interface.h"

namespace Kernel {
class KScheduler;
} // namespace Kernel

namespace Core {
class ExclusiveMonitor;
class System;
} // namespace Core

namespace Kernel {

class PhysicalCore {
public:
    PhysicalCore(std::size_t core_index_, Core::System& system_);
    ~PhysicalCore();

    YUZU_NON_COPYABLE(PhysicalCore);
    YUZU_NON_MOVEABLE(PhysicalCore);

    /// Execute JIT and return whether we ran to completion
    bool Run(Core::ARM_Interface& current_arm_interface);

    void WaitForInterrupt();

    /// Interrupt this physical core.
    void Interrupt();

    /// Clear this core's interrupt
    void ClearInterrupt();

    bool GetIsInterrupted() const {
        return m_is_interrupted;
    }

    std::size_t GetCoreIndex() const {
        return m_core_index;
    }

private:
    const std::size_t m_core_index;
    Core::System& m_system;

    std::mutex m_guard;
    std::condition_variable m_on_interrupt;
    bool m_is_interrupted{};
    Core::ARM_Interface* m_current_arm_interface{};
};

} // namespace Kernel
