// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include "common/common_types.h"

class ARM_Interface;

namespace Kernel {
class Scheduler;
}

namespace Core {

constexpr unsigned NUM_CPU_CORES{4};

class CpuBarrier {
public:
    void Rendezvous() {
        std::unique_lock<std::mutex> lock(mutex);

        --cores_waiting;
        if (!cores_waiting) {
            cores_waiting = NUM_CPU_CORES;
            condition.notify_all();
            return;
        }

        condition.wait(lock);
    }

private:
    unsigned cores_waiting{NUM_CPU_CORES};
    std::mutex mutex;
    std::condition_variable condition;
};

class Cpu {
public:
    Cpu(std::shared_ptr<CpuBarrier> cpu_barrier, size_t core_index);

    void RunLoop(bool tight_loop = true);

    void SingleStep();

    void PrepareReschedule();

    ARM_Interface& CPU() {
        return *arm_interface;
    }

    Kernel::Scheduler& Scheduler() {
        return *scheduler;
    }

    bool IsMainCore() const {
        return core_index == 0;
    }

private:
    void Reschedule();

    std::shared_ptr<ARM_Interface> arm_interface;
    std::shared_ptr<CpuBarrier> cpu_barrier;
    std::unique_ptr<Kernel::Scheduler> scheduler;

    bool reschedule_pending{};
    size_t core_index;
};

} // namespace Core
