// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/assert.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "video_core/memory_manager.h"

namespace Tegra {
namespace Engines {

#define FERMI2D_REG_INDEX(field_name)                                                              \
    (offsetof(Tegra::Engines::Fermi2D::Regs, field_name) / sizeof(u32))

class Fermi2D final {
public:
    explicit Fermi2D(MemoryManager& memory_manager);
    ~Fermi2D() = default;

    /// Write the value to the register identified by method.
    void WriteReg(u32 method, u32 value);

    struct Regs {
        static constexpr size_t NUM_REGS = 0x258;

        union {
            struct {
                INSERT_PADDING_WORDS(0x258);
            };
            std::array<u32, NUM_REGS> reg_array;
        };
    } regs{};

    MemoryManager& memory_manager;
};

#define ASSERT_REG_POSITION(field_name, position)                                                  \
    static_assert(offsetof(Fermi2D::Regs, field_name) == position * 4,                             \
                  "Field " #field_name " has invalid position")

#undef ASSERT_REG_POSITION

} // namespace Engines
} // namespace Tegra
