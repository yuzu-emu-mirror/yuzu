// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "common/fiber.h"
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

private:
    void ChannelLoop(size_t gpfifo_id, s32 channel_id);

    std::unordered_map<s32, std::shared_ptr<ChannelState>> channels;
    std::unordered_map<s32, size_t> channel_gpfifo_ids;
    std::mutex scheduling_guard;
    std::shared_ptr<Common::Fiber> master_control;
    struct GPFifoContext {
        bool is_active;
        std::shared_ptr<Common::Fiber> context;
        std::deque<CommandList> pending_work;
        std::atomic<bool> working{};
        std::mutex guard;
        s32 bind_id;
    };
    std::deque<GPFifoContext> gpfifos;
    std::deque<size_t> free_fifos;
    GPU& gpu;
    size_t current_fifo_rotation_id{};
    GPFifoContext* current_fifo{};
};

} // namespace Control

} // namespace Tegra
