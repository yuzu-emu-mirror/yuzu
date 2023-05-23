// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <iomanip>
#include <sstream>
#include <fmt/chrono.h>
#include <fmt/core.h>

#include "common/logging/log.h"
#include "common/time_zone.h"

namespace Common::TimeZone {

std::string GetDefaultTimeZone() {
    return "GMT";
}

static std::string GetOsTimeZoneOffset() {
    const std::time_t t{std::time(nullptr)};
    const std::tm tm{*std::localtime(&t)};

    return fmt::format("{:%z}", tm);
}

static int ConvertOsTimeZoneOffsetToInt(const std::string& timezone) {
    try {
        return std::stoi(timezone);
    } catch (const std::invalid_argument&) {
        LOG_CRITICAL(Common, "invalid_argument with {}!", timezone);
        return 0;
    } catch (const std::out_of_range&) {
        LOG_CRITICAL(Common, "out_of_range with {}!", timezone);
        return 0;
    }
}

std::chrono::seconds GetCurrentOffsetSeconds() {
    const int offset{ConvertOsTimeZoneOffsetToInt(GetOsTimeZoneOffset())};

    int seconds{(offset / 100) * 60 * 60}; // Convert hour component to seconds
    seconds += (offset % 100) * 60;        // Convert minute component to seconds

    return std::chrono::seconds{seconds};
}

std::string FindSystemTimeZone() {
#if defined(MINGW)
    // MinGW has broken strftime -- https://sourceforge.net/p/mingw-w64/bugs/793/
    // e.g. fmt::format("{:%z}") -- returns "Eastern Daylight Time" when it should be "-0400"
    return timezones[0];
#else
    static std::string system_time_zone_cached{};
    if (!system_time_zone_cached.empty()) {
        return system_time_zone_cached;
    }

    const auto now = std::time(nullptr);
    const struct std::tm& local = *std::localtime(&now);

    const auto system_offset = GetCurrentOffsetSeconds().count() - (local.tm_isdst ? 3600 : 0);

    int min = std::numeric_limits<int>::max();
    int min_index = -1;
    for (u32 i = 2; i < offsets.size(); i++) {
        // Skip if system is celebrating DST but considered time zone does not
        if (local.tm_isdst && !dst[i]) {
            continue;
        }

        const auto offset = offsets[i];
        const int difference = std::abs(std::abs(offset) - std::abs(system_offset));
        if (difference < min) {
            min = difference;
            min_index = i;
        }
    }

    system_time_zone_cached = timezones[min_index];
    return timezones[min_index];
#endif
}

} // namespace Common::TimeZone
