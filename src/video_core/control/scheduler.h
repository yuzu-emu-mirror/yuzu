// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <list>
#include <memory>

#include "common/common_types.h"
#include "video_core/control/channel_state.h"
#include "video_core/dma_pusher.h"

namespace Tegra {

class GPU;

namespace Control {

struct ChannelState;

class Scheduler {
public:
    explicit Scheduler(GPU& gpu_);
    ~Scheduler();

    void Init();

    void Resume();

    void Yield();

    void Push(s32 channel, CommandList&& entries);

    void DeclareChannel(std::shared_ptr<ChannelState> new_channel);

    void ChangePriority(s32 channel_id, u32 new_priority);

private:
    void ChannelLoop(size_t gpfifo_id, s32 channel_id);
    bool ScheduleLevel(std::list<size_t>& queue);
    void CheckStatus();
    bool UpdateHighestPriorityChannel();

    struct SchedulerImpl;
    std::unique_ptr<SchedulerImpl> impl;

    GPU& gpu;
};

} // namespace Control

} // namespace Tegra
