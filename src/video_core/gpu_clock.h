// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

/**
 * The GPU runs at different clockspeeds and paces than the CPU. It requires
 * its own timer in order to keep it at check. Sadly a cycle timer is unfeasible
 * for the GPU since it's pretty hard to accurately estimate the length of time
 * actions like Draw, DispatchCompute and Clear can take (varying very unproportionally)
 * Using the CPU's clock also has it's disadvantages:
 * - If the CPU is idle while the GPU is running, the timer will advance unproportionally
 * due to how CPU's idle timing works.
 * - If the GPU is synced, the timer won't advance until control is given back to
 * the CPU.
 * For all these reasons, it has been decided to use a host timer for the GPU.
 *
 **/

#pragma once

#include <chrono>

#include "common/common_types.h"

namespace Tegra {

enum GPUClockProfiles : u64 {
    Profile1 = 384000000,
    Profile2 = 768000000,
    Profile3 = 691200000,
    Profile4 = 230400000,
    Profile5 = 307000000,
    Profile6 = 460800000,
};

class GPUClock {
public:
    GPUClock();
    ~GPUClock();

    u64 GetTicks() const;

    std::chrono::nanoseconds GetNsTime() const {
        const Clock::time_point now = Clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_walltime);
    }

    std::chrono::microseconds GetUsTime() const {
        const Clock::time_point now = Clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - start_walltime);
    }

    std::chrono::milliseconds GetMsTime() const {
        const Clock::time_point now = Clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_walltime);
    }

    void SetGPUClock(const u64 new_clock) {
        gpu_clock = new_clock;
    }

private:
    using Clock = std::chrono::system_clock;
    Clock::time_point start_walltime = Clock::now();

    u64 gpu_clock{GPUClockProfiles::Profile1};
};

} // namespace Tegra
