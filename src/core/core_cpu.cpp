// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <condition_variable>
#include <mutex>

#include "common/logging/log.h"
#ifdef ARCHITECTURE_x86_64
#include "core/arm/dynarmic/arm_dynarmic.h"
#endif
#include "core/arm/unicorn/arm_unicorn.h"
#include "core/core_cpu.h"
#include "core/core_timing.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/scheduler.h"
#include "core/hle/kernel/thread.h"
#include "core/settings.h"

namespace Core {

Cpu::Cpu(std::shared_ptr<CpuBarrier> cpu_barrier, size_t core_index)
    : cpu_barrier{std::move(cpu_barrier)}, core_index{core_index} {

    if (Settings::values.use_cpu_jit) {
#ifdef ARCHITECTURE_x86_64
        arm_interface = std::make_shared<ARM_Dynarmic>();
#else
        cpu_core = std::make_shared<ARM_Unicorn>();
        NGLOG_WARNING(Core, "CPU JIT requested, but Dynarmic not available");
#endif
    } else {
        arm_interface = std::make_shared<ARM_Unicorn>();
    }

    scheduler = std::make_shared<Kernel::Scheduler>(arm_interface.get());
}

void Cpu::RunLoop(bool tight_loop) {
    // Wait for all other CPU cores to complete the previous slice, such that they run in lock-step
    cpu_barrier->Rendezvous();

    // If we don't have a currently active thread then don't execute instructions,
    // instead advance to the next event and try to yield to the next thread
    if (Kernel::GetCurrentThread() == nullptr) {
        NGLOG_TRACE(Core, "Core-{} idling", core_index);

        if (IsMainCore()) {
            CoreTiming::Idle();
            CoreTiming::Advance();
        }

        PrepareReschedule();
    } else {
        if (IsMainCore()) {
            CoreTiming::Advance();
        }

        if (tight_loop) {
            arm_interface->Run();
        } else {
            arm_interface->Step();
        }
    }

    Reschedule();
}

void Cpu::SingleStep() {
    return RunLoop(false);
}

void Cpu::PrepareReschedule() {
    arm_interface->PrepareReschedule();
    reschedule_pending = true;
}

void Cpu::Reschedule() {
    if (!reschedule_pending) {
        return;
    }

    reschedule_pending = false;
    scheduler->Reschedule();
}

} // namespace Core
