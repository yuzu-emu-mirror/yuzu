// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "common/common_types.h"
#include "video_core/textures/texture.h"

namespace Tegra {
namespace Texture {

/**
 * Decodes a swizzled texture into a RGBA8888 texture.
 */
std::vector<u8> DecodeTexture(VAddr address, TextureFormat format, u32 width, u32 height);

} // namespace Texture
} // namespace Tegra
