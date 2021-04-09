// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/fs/path_util.h"

namespace Common::FS {

namespace fs = std::filesystem;

std::string PathToUTF8String(const fs::path& path) {
    const auto utf8_string = path.u8string();

    return std::string{utf8_string.begin(), utf8_string.end()};
}

} // namespace Common::FS
