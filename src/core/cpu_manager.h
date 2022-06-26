// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <csetjmp>
#include <functional>
#include <memory>
#include <thread>

#include "common/fiber.h"
#include "common/thread.h"
#include "core/hardware_properties.h"

namespace Common {
class Event;
class Fiber;
} // namespace Common

namespace Core {

class System;

class CpuManager {
public:
    explicit CpuManager(System& system_);
    CpuManager(const CpuManager&) = delete;
    CpuManager(CpuManager&&) = delete;

    ~CpuManager();

    CpuManager& operator=(const CpuManager&) = delete;
    CpuManager& operator=(CpuManager&&) = delete;

    void OnGpuReady() {
        gpu_barrier->Sync();
    }

    void WaitForAndHandleInterrupt();
    void Initialize();
    void Shutdown();

    std::function<void()> GetGuestActivateFunc() {
        return [this] { GuestActivate(); };
    }
    std::function<void()> GetGuestThreadFunc() {
        return [this] { RunGuestThread(); };
    }
    std::function<void()> GetShutdownThreadStartFunc() {
        return [this] { ShutdownThread(); };
    }

private:
    static void ThreadStart(std::stop_token stop_token, CpuManager& cpu_manager, std::size_t core);

    void GuestActivate();
    void RunGuestThread();
    void HandleInterrupt();
    void ShutdownThread();

    void RunThread(std::size_t core);

    struct CoreData {
        std::shared_ptr<Common::Fiber> host_context;
        std::jthread host_thread;
    };

    std::unique_ptr<Common::Barrier> gpu_barrier{};
    std::array<CoreData, Core::Hardware::NUM_CPU_CORES> core_data{};
    System& system;
};

} // namespace Core
