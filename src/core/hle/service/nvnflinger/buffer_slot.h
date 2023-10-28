// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2014 The Android Open Source Project
// SPDX-License-Identifier: GPL-3.0-or-later
// Parts of this implementation were based on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/include/gui/BufferSlot.h

#pragma once

#include <memory>

#include "common/common_types.h"
#include "core/hle/service/nvnflinger/ui/fence.h"

namespace Service::Nvidia::NvCore {
class NvMap;
}

namespace Service::android {

struct GraphicBuffer;

enum class BufferState : u32 {
    Free = 0,
    Dequeued = 1,
    Queued = 2,
    Acquired = 3,
};

class MapHandle final {
public:
    MapHandle();
    ~MapHandle();

    MapHandle(const MapHandle& rhs) = delete;
    MapHandle& operator=(const MapHandle& rhs);

    void emplace(Service::Nvidia::NvCore::NvMap& nvmap, u32 nvmap_id);
    void reset();

private:
    Service::Nvidia::NvCore::NvMap* m_nvmap;
    u32 m_nvmap_id;
};

struct BufferSlot final {
    BufferSlot() = default;

    std::shared_ptr<GraphicBuffer> graphic_buffer;
    MapHandle map_handle;
    BufferState buffer_state{BufferState::Free};
    bool request_buffer_called{};
    u64 frame_number{};
    Fence fence;
    bool acquire_called{};
    bool attached_by_consumer{};
    bool is_preallocated{};
};

} // namespace Service::android
