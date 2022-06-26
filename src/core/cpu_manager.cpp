// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/fiber.h"
#include "common/microprofile.h"
#include "common/scope_exit.h"
#include "common/thread.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/cpu_manager.h"
#include "core/hle/kernel/k_interrupt_manager.h"
#include "core/hle/kernel/k_scheduler.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/physical_core.h"
#include "video_core/gpu.h"

namespace Core {

CpuManager::CpuManager(System& system_) : system{system_} {}
CpuManager::~CpuManager() = default;

void CpuManager::ThreadStart(std::stop_token stop_token, CpuManager& cpu_manager,
                             std::size_t core) {
    cpu_manager.RunThread(core);
}

void CpuManager::Initialize() {
    const auto num_cores{Core::Hardware::NUM_CPU_CORES};
    gpu_barrier = std::make_unique<Common::Barrier>(num_cores + 1);

    for (std::size_t core = 0; core < num_cores; core++) {
        core_data[core].host_thread = std::jthread(ThreadStart, std::ref(*this), core);
    }
}

void CpuManager::Shutdown() {
    for (std::size_t core = 0; core < Core::Hardware::NUM_CPU_CORES; core++) {
        if (core_data[core].host_thread.joinable()) {
            core_data[core].host_thread.join();
        }
    }
}

void CpuManager::WaitForAndHandleInterrupt() {
    auto& kernel = system.Kernel();
    auto& physical_core = kernel.CurrentPhysicalCore();

    ASSERT(Kernel::GetCurrentThread(kernel).GetDisableDispatchCount() == 1);

    if (!physical_core.IsInterrupted()) {
        physical_core.Idle();
    }

    HandleInterrupt();
}

void CpuManager::HandleInterrupt() {
    auto& kernel = system.Kernel();
    auto core_index = kernel.CurrentPhysicalCoreIndex();

    Kernel::KInterruptManager::HandleInterrupt(kernel, static_cast<s32>(core_index));
}

void CpuManager::GuestActivate() {
    // Similar to the HorizonKernelMain callback in HOS
    auto& kernel = system.Kernel();
    auto* scheduler = kernel.CurrentScheduler();

    kernel.SetCurrentEmuThread(scheduler->GetSchedulerCurrentThread());
    scheduler->Activate();

    UNREACHABLE();
}

void CpuManager::RunGuestThread() {
    // Similar to UserModeThreadStarter in HOS
    auto& kernel = system.Kernel();
    kernel.GetCurrentEmuThread()->EnableDispatch();

    while (true) {
        auto* physical_core = &kernel.CurrentPhysicalCore();
        while (!physical_core->IsInterrupted()) {
            physical_core->Run();
            physical_core = &kernel.CurrentPhysicalCore();
        }

        HandleInterrupt();
    }

    UNREACHABLE();
}

void CpuManager::ShutdownThread() {
    auto& kernel = system.Kernel();
    auto* thread = kernel.GetCurrentEmuThread();
    auto core = kernel.CurrentPhysicalCoreIndex();

    Common::Fiber::YieldTo(thread->GetHostContext(), *core_data[core].host_context);
    UNREACHABLE();
}

void CpuManager::RunThread(std::size_t core) {
    /// Initialization
    system.RegisterCoreThread(core);

    auto name{fmt::format("yuzu:CPUCore_{}", core)};
    MicroProfileOnThreadCreate(name.c_str());
    Common::SetCurrentThreadName(name.c_str());
    Common::SetCurrentThreadPriority(Common::ThreadPriority::High);

    auto& data{core_data[core]};
    data.host_context = Common::Fiber::ThreadToFiber();

    // Cleanup
    SCOPE_EXIT({
        data.host_context->Exit();
        MicroProfileOnThreadExit();
    });

    // Running
    gpu_barrier->Sync();

    auto& kernel{system.Kernel()};
    auto* main_thread{kernel.CurrentScheduler()->GetSchedulerCurrentThread()};

    Common::Fiber::YieldTo(data.host_context, *main_thread->GetHostContext());
}

} // namespace Core
