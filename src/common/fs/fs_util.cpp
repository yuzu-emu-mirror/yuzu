// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <iostream>
#include <string>
#ifdef _WIN32
#include <windows.h> // For MultiByteToWideChar (cannon UTF-8 with Windows)
#endif

/* TODO: Add support for 'https://github.com/unicode-org/icu' to handle UTF-8.
 *      This will be necessary for support UTF-8 cannon on Linux and macOS conversions.
 */

#include "common/fs/fs_util.h"
#include "common/polyfill_ranges.h"

namespace Common::FS {

std::u8string ToU8String(std::wstring_view w_string) {
    return std::u8string{reinterpret_cast<const char8_t*>(w_string.data())};
}

std::u8string ToU8String(std::string_view string) {
    return std::u8string{string.begin(), string.end()};
}

std::u8string BufferToU8String(std::span<const u8> buffer) {
    return std::u8string{buffer.begin(), std::ranges::find(buffer, u8{0})};
}

std::u8string_view BufferToU8StringView(std::span<const u8> buffer) {
    return std::u8string_view{reinterpret_cast<const char8_t*>(buffer.data())};
}

std::wstring ToWString(std::u8string_view utf8_string) {
#ifdef _WIN32
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(utf8_string.data()), -1, NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(utf8_string.data()), -1, &wstr[0], size_needed);
    return wstr;
#else
    return std::wstring{utf8_string.begin(), utf8_string.end()};
#endif
}

std::string ToUTF8String(std::u8string_view u8_string) {
    return std::string{u8_string.begin(), u8_string.end()};
}

std::string BufferToUTF8String(std::span<const u8> buffer) {
    return std::string{buffer.begin(), std::ranges::find(buffer, u8{0})};
}

std::string_view BufferToUTF8StringView(std::span<const u8> buffer) {
    return std::string_view{reinterpret_cast<const char*>(buffer.data())};
}

std::string PathToUTF8String(const std::filesystem::path& path) {
    return ToUTF8String(path.u8string());
}

} // namespace Common::FS
