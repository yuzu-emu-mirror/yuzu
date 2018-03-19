// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "video_core/memory_manager.h"

namespace Tegra {
namespace Texture {

enum class TextureFormat : u32 {
    DXT1 = 0x24,
};

union TextureHandle {
    u32 raw;
    BitField<0, 20, u32> tic_id;
    BitField<20, 12, u32> tsc_id;
};

struct TICEntry {
    union {
        u32 raw;
        BitField<0, 7, TextureFormat> format;
        BitField<7, 3, u32> r_type;
        BitField<10, 3, u32> g_type;
        BitField<13, 3, u32> b_type;
        BitField<16, 3, u32> a_type;
    };
    u32 address_low;
    u16 address_high;
    INSERT_PADDING_BYTES(6);
    u16 width_minus_1;
    INSERT_PADDING_BYTES(2);
    u16 height_minus_1;
    INSERT_PADDING_BYTES(10);

    GPUVAddr Address() const {
        return static_cast<GPUVAddr>((static_cast<GPUVAddr>(address_high) << 32) | address_low);
    }

    u32 Width() const {
        return width_minus_1 + 1;
    }

    u32 Height() const {
        return height_minus_1 + 1;
    }
};
static_assert(sizeof(TICEntry) == 0x20, "TICEntry has wrong size");

} // namespace Texture
} // namespace Tegra
