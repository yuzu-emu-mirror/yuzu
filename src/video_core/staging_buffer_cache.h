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
    using Cache = std::unordered_map<u32, std::vector<std::unique_ptr<StagingBufferType>>>;

public:
    explicit StagingBufferCache(bool can_flush_aot) : can_flush_aot{can_flush_aot} {}
    virtual ~StagingBufferCache() = default;

    [[nodiscard]] StagingBufferType& GetWriteBuffer(std::size_t size) {
        return GetBuffer(size, false);
    }

    [[nodiscard]] StagingBufferType& GetReadBuffer(std::size_t size) {
        return GetBuffer(size, true);
    }

    [[nodiscard]] bool CanFlushAheadOfTime() const {
        return can_flush_aot;
    }

protected:
    virtual std::unique_ptr<StagingBufferType> CreateBuffer(std::size_t size, bool is_flush) = 0;

private:
    StagingBufferType& GetBuffer(std::size_t size, bool is_flush) {
        const u32 ceil = Common::Log2Ceil64(size);
        auto& buffers = (is_flush ? flush_cache : upload_cache)[ceil];
        const auto it = std::find_if(buffers.begin(), buffers.end(),
                                     [](auto& buffer) { return buffer->IsAvailable(); });
        if (it != buffers.end()) {
            return **it;
        }
        return *buffers.emplace_back(CreateBuffer(1ULL << ceil, is_flush));
    }

    bool can_flush_aot{};
    Cache upload_cache;
    Cache flush_cache;
};

} // namespace VideoCommon
