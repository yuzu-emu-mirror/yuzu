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
    A8R8G8B8 = 8,
    DXT1 = 0x24,
};

enum class TextureType : u32 {
    Texture1D = 0,
    Texture2D = 1,
    Texture3D = 2,
    TextureCubemap = 3,
    Texture1DArray = 4,
    Texture2DArray = 5,
    Texture1DBuffer = 6,
    Texture2DNoMipmap = 7,
    TextureCubeArray = 8,
};

enum class TICHeaderVersion : u32 {
    OneDBuffer = 0,
    PitchColorKey = 1,
    Pitch = 2,
    BlockLinear = 3,
    BlockLinearColorKey = 4,
};

union TextureHandle {
    u32 raw;
    BitField<0, 20, u32> tic_id;
    BitField<20, 12, u32> tsc_id;
};
static_assert(sizeof(TextureHandle) == 4, "TextureHandle has wrong size");

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
    union {
        BitField<0, 16, u32> address_high;
        BitField<21, 3, TICHeaderVersion> header_version;
    };
    INSERT_PADDING_BYTES(4);
    union {
        BitField<0, 16, u32> width_minus_1;
        BitField<23, 4, TextureType> texture_type;
    };
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

/// Returns the number of bytes per pixel of the input texture format.
u32 BytesPerPixel(TextureFormat format);

} // namespace Texture
} // namespace Tegra
