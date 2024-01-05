// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>

#include "common/assert.h"
#include "video_core/control/channel_state.h"
#include "video_core/control/scheduler.h"
#include "video_core/gpu.h"

namespace Tegra::Control {
Scheduler::Scheduler(GPU& gpu_) : gpu{gpu_} {}

Scheduler::~Scheduler() = default;

void Scheduler::Init() {
    master_control = Common::Fiber::ThreadToFiber();
}

void Scheduler::Resume() {
    bool nothing_pending;
    do {
        nothing_pending = true;
        current_fifo = nullptr;
        {
            std::unique_lock lk(scheduling_guard);
            size_t num_iters = gpfifos.size();
            for (size_t i = 0; i < num_iters; i++) {
                size_t current_id = (current_fifo_rotation_id + i) % gpfifos.size();
                auto& fifo = gpfifos[current_id];
                if (!fifo.is_active) {
                    continue;
                }
                std::scoped_lock lk2(fifo.guard);
                if (!fifo.pending_work.empty() || fifo.working.load(std::memory_order_acquire)) {
                    current_fifo = &fifo;
                    current_fifo_rotation_id = current_id;
                    nothing_pending = false;
                    break;
                }
            }
        }
        if (current_fifo) {
            Common::Fiber::YieldTo(master_control, *current_fifo->context);
            current_fifo = nullptr;
        }
    } while (!nothing_pending);
}

void Scheduler::Yield() {
    ASSERT(current_fifo != nullptr);
    Common::Fiber::YieldTo(current_fifo->context, *master_control);
    gpu.BindChannel(current_fifo->bind_id);
}

void Scheduler::Push(s32 channel, CommandList&& entries) {
    std::unique_lock lk(scheduling_guard);
    auto it = channel_gpfifo_ids.find(channel);
    ASSERT(it != channel_gpfifo_ids.end());
    auto gpfifo_id = it->second;
    auto& fifo = gpfifos[gpfifo_id];
    {
        std::scoped_lock lk2(fifo.guard);
        fifo.pending_work.emplace_back(std::move(entries));
    }
}

void Scheduler::ChannelLoop(size_t gpfifo_id, s32 channel_id) {
    gpu.BindChannel(channel_id);
    auto& fifo = gpfifos[gpfifo_id];
    while (true) {
        auto* channel_state = channels[channel_id].get();
        fifo.guard.lock();
        while (!fifo.pending_work.empty()) {
            {

                fifo.working.store(true, std::memory_order_release);
                CommandList&& entries = std::move(fifo.pending_work.front());
                channel_state->dma_pusher->Push(std::move(entries));
                fifo.pending_work.pop_front();
            }
            fifo.guard.unlock();
            channel_state->dma_pusher->DispatchCalls();
            fifo.guard.lock();
        }
        fifo.working.store(false, std::memory_order_relaxed);
        fifo.guard.unlock();
        Common::Fiber::YieldTo(fifo.context, *master_control);
        gpu.BindChannel(channel_id);
    }
}

void Scheduler::DeclareChannel(std::shared_ptr<ChannelState> new_channel) {
    s32 channel = new_channel->bind_id;
    std::unique_lock lk(scheduling_guard);
    channels.emplace(channel, new_channel);
    size_t new_fifo_id;
    if (!free_fifos.empty()) {
        new_fifo_id = free_fifos.front();
        free_fifos.pop_front();
    } else {
        new_fifo_id = gpfifos.size();
        gpfifos.emplace_back();
    }
    auto& new_fifo = gpfifos[new_fifo_id];
    channel_gpfifo_ids[channel] = new_fifo_id;
    new_fifo.is_active = true;
    new_fifo.bind_id = channel;
    new_fifo.pending_work.clear();
    std::function<void()> callback = std::bind(&Scheduler::ChannelLoop, this, new_fifo_id, channel);
    new_fifo.context = std::make_shared<Common::Fiber>(std::move(callback));
}

} // namespace Tegra::Control
