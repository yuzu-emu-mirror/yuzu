// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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

    std::chrono::nanoseconds GetNsTime() const {
        Clock::time_point now = Clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_walltime);
    }

    std::chrono::microseconds GetUsTime() const {
        Clock::time_point now = Clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - start_walltime);
    }

    std::chrono::milliseconds GetMsTime() const {
        Clock::time_point now = Clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_walltime);
    }

    u64 GetTicks() const;

    void SetGPUClock(const u64 new_clock) {
        gpu_clock = new_clock;
    }

private:
    using Clock = std::chrono::system_clock;
    Clock::time_point start_walltime = Clock::now();

    u64 gpu_clock{GPUClockProfiles::Profile1};
};

} // namespace Tegra
