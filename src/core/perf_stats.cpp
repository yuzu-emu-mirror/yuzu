// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <chrono>
#include <mutex>
#include <thread>
#include "common/math_util.h"
#include "core/perf_stats.h"
#include "core/settings.h"

using namespace std::chrono_literals;
using DoubleSecs = std::chrono::duration<double, std::chrono::seconds::period>;
using std::chrono::duration_cast;
using std::chrono::microseconds;

namespace Core {

void PerfStats::BeginSystemFrame() {
    std::lock_guard<std::mutex> lock(object_mutex);

    frame_begin = Clock::now();
}

void PerfStats::EndSystemFrame() {
    std::lock_guard<std::mutex> lock(object_mutex);

    auto frame_end = Clock::now();
    accumulated_frametime += frame_end - frame_begin;
    system_frames += 1;

    previous_frame_length = frame_end - previous_frame_end;
    previous_frame_end = frame_end;
}

void PerfStats::EndGameFrame() {
    std::lock_guard<std::mutex> lock(object_mutex);

    game_frames += 1;
}

PerfStats::Results PerfStats::GetAndResetStats(microseconds current_system_time_us) {
    std::lock_guard<std::mutex> lock(object_mutex);

    const auto now = Clock::now();
    // Walltime elapsed since stats were reset
    const auto interval = duration_cast<DoubleSecs>(now - reset_point).count();

    const auto system_us_per_second = (current_system_time_us - reset_point_system_us) / interval;

    Results results{};
    results.system_fps = static_cast<double>(system_frames) / interval;
    results.game_fps = static_cast<double>(game_frames) / interval;
    results.frametime = duration_cast<DoubleSecs>(accumulated_frametime).count() /
                        static_cast<double>(system_frames);
    results.emulation_speed = system_us_per_second.count() / 1'000'000.0;

    // Reset counters
    reset_point = now;
    reset_point_system_us = current_system_time_us;
    accumulated_frametime = Clock::duration::zero();
    system_frames = 0;
    game_frames = 0;

    return results;
}

double PerfStats::GetLastFrameTimeScale() {
    std::lock_guard<std::mutex> lock(object_mutex);

    constexpr double FRAME_LENGTH = 1.0 / 60;
    return duration_cast<DoubleSecs>(previous_frame_length).count() / FRAME_LENGTH;
}

void FrameLimiter::DoFrameLimiting(microseconds current_system_time_us) {
    // Max lag caused by slow frames. Can be adjusted to compensate for too many slow frames. Higher
    // values increase the time needed to recover and limit framerate again after spikes.
    constexpr microseconds MAX_LAG_TIME_US = 25us;

    if (!Settings::values.toggle_framelimit) {
        return;
    }

    auto now = Clock::now();

    frame_limiting_delta_err += current_system_time_us - previous_system_time_us;
    frame_limiting_delta_err -= duration_cast<microseconds>(now - previous_walltime);
    frame_limiting_delta_err =
        std::clamp(frame_limiting_delta_err, -MAX_LAG_TIME_US, MAX_LAG_TIME_US);

    if (frame_limiting_delta_err > microseconds::zero()) {
        std::this_thread::sleep_for(frame_limiting_delta_err);

        auto now_after_sleep = Clock::now();
        frame_limiting_delta_err -= duration_cast<microseconds>(now_after_sleep - now);
        now = now_after_sleep;
    }

    previous_system_time_us = current_system_time_us;
    previous_walltime = now;
}

} // namespace Core
