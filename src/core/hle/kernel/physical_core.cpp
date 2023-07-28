// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/arm/dynarmic/arm_dynarmic_32.h"
#include "core/arm/dynarmic/arm_dynarmic_64.h"
#include "core/core.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/physical_core.h"

namespace Kernel {

PhysicalCore::PhysicalCore(std::size_t core_index, Core::System& system)
    : m_core_index{core_index}, m_system{system} {}

PhysicalCore::~PhysicalCore() = default;

bool PhysicalCore::Run(Core::ARM_Interface& current_arm_interface) {
    // SAFETY: we need to lock around storing the JIT instance member to serialize
    // access with another thread that wants to send us an interrupt. Otherwise we
    // may end up sending an interrupt to an instance that is not current.

    // Mark the instance as current.
    {
        std::scoped_lock lk{m_guard};

        // If this core is already interrupted, return immediately.
        if (m_is_interrupted) {
            return false;
        }

        m_current_arm_interface = std::addressof(current_arm_interface);
    }

    // Run the instance.
    current_arm_interface.Run();
    current_arm_interface.ClearExclusiveState();

    {
        std::scoped_lock lk{m_guard};

        // Mark the instance as no longer current.
        m_current_arm_interface = nullptr;

        // Return whether we ran to completion.
        return !m_is_interrupted;
    }
}

void PhysicalCore::WaitForInterrupt() {
    // Wait for a signal.
    std::unique_lock lk{m_guard};
    m_on_interrupt.wait(lk, [this] { return m_is_interrupted; });
}

void PhysicalCore::Interrupt() {
    {
        std::scoped_lock lk{m_guard};

        // Mark as interrupted.
        m_is_interrupted = true;

        // If we are currently executing code, interrupt the JIT.
        if (m_current_arm_interface) {
            m_current_arm_interface->SignalInterrupt();
        }
    }

    // Signal.
    m_on_interrupt.notify_all();
}

void PhysicalCore::ClearInterrupt() {
    std::scoped_lock lk{m_guard};

    // Remove interrupt flag.
    m_is_interrupted = false;
}

} // namespace Kernel
