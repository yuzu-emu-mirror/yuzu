// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include "common/bit_util.h"
#include "common/common_types.h"

namespace VideoCommon {

template <typename StagingBufferType>
class StagingBufferCache {
public:
    explicit StagingBufferCache() = default;
    ~StagingBufferCache() = default;

    StagingBufferType& GetBuffer(std::size_t size) {
        const u32 ceil = Common::Log2Ceil64(size);
        auto& buffers = cache[ceil];
        const auto it = std::find_if(buffers.begin(), buffers.end(),
                                     [](const auto& buffer) { return buffer->IsAvailable(); });
        if (it != buffers.end()) {
            return **it;
        }
        const std::size_t buf_size = 1ULL << ceil;
        return *buffers.emplace_back(std::make_unique<StagingBufferType>(buf_size));
    }

private:
    std::unordered_map<u32, std::vector<std::unique_ptr<StagingBufferType>>> cache;
};

} // namespace VideoCommon
