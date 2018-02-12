// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "video_core/memory_manager.h"

namespace Tegra {
namespace Engines {

class Maxwell3D final {
public:
    Maxwell3D(MemoryManager& memory_manager);
    ~Maxwell3D() = default;

    /// Write the value to the register identified by method.
    void WriteReg(u32 method, u32 value);

    /// Register structure of the Maxwell3D engine.
    /// TODO(Subv): This structure will need to be made bigger as more registers are discovered.
    struct Regs {
        static constexpr size_t NUM_REGS = 0xE36;

        enum class QueryMode : u32 {
            Write = 0,
            Sync = 1,
        };

        union {
            struct {
                INSERT_PADDING_WORDS(0x6C0);
                struct {
                    u32 query_address_high;
                    u32 query_address_low;
                    u32 query_sequence;
                    union {
                        u32 raw;
                        BitField<0, 2, QueryMode> mode;
                        BitField<4, 1, u32> fence;
                        BitField<12, 4, u32> unit;
                    } query_get;

                    GPUVAddr QueryAddress() const {
                        return static_cast<GPUVAddr>(
                            (static_cast<GPUVAddr>(query_address_high) << 32) | query_address_low);
                    }
                } query;
                INSERT_PADDING_WORDS(0x772);
            };
            std::array<u32, NUM_REGS> reg_array;
        };
    } regs{};

    static_assert(sizeof(Regs) == Regs::NUM_REGS * sizeof(u32), "Maxwell3D Regs has wrong size");

private:
    /// Handles a write to the QUERY_GET register.
    void ProcessQueryGet();

    MemoryManager& memory_manager;
};

#define ASSERT_REG_POSITION(field_name, position)                                                  \
    static_assert(offsetof(Maxwell3D::Regs, field_name) == position * 4,                           \
                  "Field " #field_name " has invalid position")

ASSERT_REG_POSITION(query, 0x6C0);

#undef ASSERT_REG_POSITION

} // namespace Engines
} // namespace Tegra
