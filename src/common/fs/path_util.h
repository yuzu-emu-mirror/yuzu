// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <filesystem>

namespace Common::FS {

/**
 * Converts a filesystem path to a UTF-8 encoded std::string.
 *
 * @param path Filesystem path
 *
 * @returns UTF-8 encoded std::string.
 */
[[nodiscard]] std::string PathToUTF8String(const std::filesystem::path& path);

} // namespace Common::FS
