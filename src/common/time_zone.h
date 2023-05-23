// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <chrono>
#include <string>

namespace Common::TimeZone {

// Time zone strings
constexpr std::array timezones{
    "GMT",       "GMT",       "CET", "CST6CDT", "Cuba",    "EET",    "Egypt",     "Eire",
    "EST",       "EST5EDT",   "GB",  "GB-Eire", "GMT",     "GMT+0",  "GMT-0",     "GMT0",
    "Greenwich", "Hongkong",  "HST", "Iceland", "Iran",    "Israel", "Jamaica",   "Japan",
    "Kwajalein", "Libya",     "MET", "MST",     "MST7MDT", "Navajo", "NZ",        "NZ-CHAT",
    "Poland",    "Portugal",  "PRC", "PST8PDT", "ROC",     "ROK",    "Singapore", "Turkey",
    "UCT",       "Universal", "UTC", "W-SU",    "WET",     "Zulu",
};

// Time zone offset in seconds from GMT
constexpr std::array offsets{
    0,     0,     3600,  -21600, -19768, 7200,   7509,   -1521, -18000, -18000, -75,    -75,
    0,     0,     0,     0,      0,      27402,  -36000, -968,  12344,  8454,   -18430, 33539,
    40160, 3164,  3600,  -25200, -25200, -25196, 41944,  44028, 5040,   -2205,  29143,  -28800,
    29160, 30472, 24925, 6952,   0,      0,      0,      9017,  0,      0,
};

// If the time zone recognizes Daylight Savings Time
constexpr std::array dst{
    false, false, true,  true,  true,  true,  true,  true,  false, true,  true, true,
    false, false, false, false, false, true,  false, false, true,  true,  true, true,
    false, true,  true,  false, true,  true,  true,  true,  true,  true,  true, true,
    true,  true,  true,  true,  false, false, false, true,  true,  false,
};

/// Gets the default timezone, i.e. "GMT"
[[nodiscard]] std::string GetDefaultTimeZone();

/// Gets the offset of the current timezone (from the default), in seconds
[[nodiscard]] std::chrono::seconds GetCurrentOffsetSeconds();

/// Searches time zone offsets for the closest offset to the system time zone
[[nodiscard]] std::string FindSystemTimeZone();

} // namespace Common::TimeZone
