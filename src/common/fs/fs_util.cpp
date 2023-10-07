// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <iostream>
#include <string>
#ifdef _WIN32
#include <windows.h> // For MultiByteToWideChar (cannon UTF-8 with Windows)
#endif

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
    int size_needed = MultiByteToWideChar(
        CP_UTF8, 0, reinterpret_cast<const char*>(utf8_string.data()), -1, NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(utf8_string.data()), -1, &wstr[0],
                        size_needed);
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

std::u8string UTF8FilenameSantizer(std::u8string u8filename) {

    std::u8string u8path_santized(u8filename);

    size_t eSizeSanitized = u8path_santized.size();

    /* Special case for ":", for example: 'Example: The Test' --> 'Example - The Test' or 'Example :
     * The Test' --> 'Example - The Test'. */
    for (size_t i = 0; i < eSizeSanitized; i++) {

        switch (u8path_santized[i]) {
        case u8':':
            if (i == 0 || i == eSizeSanitized - 1) {
                u8path_santized.replace(i, 1, u8"_");
            } else if (u8path_santized[i - 1] == u8' ') {
                u8path_santized.replace(i, 1, u8"-");
            } else {
                u8path_santized.replace(i, 1, u8" -");
                eSizeSanitized++;
            }
            break;
        case u8'\\':
        case u8'/':
        case u8'*':
        case u8'?':
        case u8'\"':
        case u8'<':
        case u8'>':
        case u8'|':
        case u8'\0':
            u8path_santized.replace(i, 1, u8"_");
            break;
        default:
            break;
        }
    }

    // Delete duplicated spaces || Delete duplicated dots (MacOS i think)
    for (size_t i = 0; i < eSizeSanitized; i++) {
        if ((u8path_santized[i] == u8' ' && u8path_santized[i + 1] == u8' ') ||
            (u8path_santized[i] == u8'.' && u8path_santized[i + 1] == u8'.')) {
            u8path_santized.erase(i, 1);
            i--;
        }
    }

    // Delete all spaces and dots at the end (Windows almost)
    while (u8path_santized.back() == u8' ' || u8path_santized.back() == u8'.') {
        u8path_santized.pop_back();
    }

    if (u8path_santized.empty()) {
        return u8"";
    }

    return u8path_santized;
}

} // namespace Common::FS
