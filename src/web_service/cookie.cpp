// SPDX-FileCopyrightText: 2023 Yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <sstream>

#include "common/string_util.h"
#include "cookie.h"

namespace WebService {

#define TM_YEAR_BASE 1900

Cookie::Cookie(std::string name, std::string value)
    : m_name{std::move(name)}, m_value{std::move(value)} {}

bool Cookie::IsSecure() const {
    return m_secure;
}

void Cookie::SetSecure(bool enable) {
    m_secure = enable;
}

bool Cookie::IsHttpOnly() const {
    return m_http_only;
}

void Cookie::SetHttpOnly(bool enable) {
    m_http_only = enable;
}

Cookie::SameSite Cookie::SameSitePolicy() const {
    return m_same_site;
}

void Cookie::SetSameSitePolicy(Cookie::SameSite same_site) {
    m_same_site = same_site;
}

std::tm Cookie::ExpirationDate() const {
    return m_expiration_date;
}

void Cookie::SetExpirationDate(std::tm date) {
    m_expiration_date = std::move(date);
}

std::string Cookie::Domain() const {
    return m_domain;
}

void Cookie::SetDomain(std::string domain_) {
    m_domain = std::move(domain_);
}

std::string Cookie::Path() const {
    return m_path;
}

void Cookie::SetPath(std::string path_) {
    m_path = std::move(path_);
}

std::string Cookie::Name() const {
    return m_name;
}

void Cookie::SetName(std::string cookie_name) {
    m_name = std::move(cookie_name);
}

std::string Cookie::Value() const {
    return m_value;
}

void Cookie::SetValue(std::string value_) {
    m_value = std::move(value_);
}

Cookie::SameSite Cookie::SameSiteFromString(std::string_view str) {
    if (str == "none")
        return Cookie::SameSite::None;
    if (str == "lax")
        return Cookie::SameSite::Lax;
    if (str == "strict")
        return Cookie::SameSite::Strict;
    return Cookie::SameSite::Default;
}

std::string Cookie::SameSiteToString(SameSite same_site) {
    switch (same_site) {
    case Cookie::SameSite::None:
        return "None";
    case Cookie::SameSite::Lax:
        return "Lax";
    case Cookie::SameSite::Strict:
        return "Strict";
    case Cookie::SameSite::Default:
        break;
    }
    return "";
}

/*
 * see: https://searchfox.org/mozilla-central/source/nsprpub/pr/src/misc/prtime.c#931
 * We only recognize the abbreviations of a small subset of time zones
 * in North America, Europe, and Japan.
 *
 * PST/PDT: Pacific Standard/Daylight Time
 * MST/MDT: Mountain Standard/Daylight Time
 * CST/CDT: Central Standard/Daylight Time
 * EST/EDT: Eastern Standard/Daylight Time
 * AST: Atlantic Standard Time
 * NST: Newfoundland Standard Time
 * GMT: Greenwich Mean Time
 * BST: British Summer Time
 * MET: Middle Europe Time
 * EET: Eastern Europe Time
 * JST: Japan Standard Time
 */
enum TIME_TOKEN {
    TT_UNKNOWN,

    TT_SUN,
    TT_MON,
    TT_TUE,
    TT_WED,
    TT_THU,
    TT_FRI,
    TT_SAT,

    TT_JAN,
    TT_FEB,
    TT_MAR,
    TT_APR,
    TT_MAY,
    TT_JUN,
    TT_JUL,
    TT_AUG,
    TT_SEP,
    TT_OCT,
    TT_NOV,
    TT_DEC,

    TT_PST,
    TT_PDT,
    TT_MST,
    TT_MDT,
    TT_CST,
    TT_CDT,
    TT_EST,
    TT_EDT,
    TT_AST,
    TT_NST,
    TT_GMT,
    TT_BST,
    TT_MET,
    TT_EET,
    TT_JST
};

/*
 * see: https://searchfox.org/mozilla-central/source/nsprpub/pr/src/misc/prtime.c#976
 * This parses a time/date string into a std::tm
 * It returns true on success,
 * false if the time/date string can't be parsed.
 *
 * Many formats are handled, including:
 *
 *   14 Apr 89 03:20:12
 *   14 Apr 89 03:20 GMT
 *   Fri, 17 Mar 89 4:01:33
 *   Fri, 17 Mar 89 4:01 GMT
 *   Mon Jan 16 16:12 PDT 1989
 *   Mon Jan 16 16:12 +0130 1989
 *   6 May 1992 16:41-JST (Wednesday)
 *   22-AUG-1993 10:59:12.82
 *   22-AUG-1993 10:59pm
 *   22-AUG-1993 12:59am
 *   22-AUG-1993 12:59 PM
 *   Friday, August 04, 1995 3:54 PM
 *   06/21/95 04:24:34 PM
 *   20/06/95 21:07
 *   95-06-08 19:32:48 EDT
 */
bool Cookie::ParseTimeString(const std::string& string, std::tm* result) {
    TIME_TOKEN dotw = TT_UNKNOWN;
    TIME_TOKEN month = TT_UNKNOWN;
    TIME_TOKEN zone = TT_UNKNOWN;
    const char* zone_name = "";
    int zone_offset = -1;
    int dst_offset = 0;
    int date = -1;
    int year = -1;
    int hour = -1;
    int min = -1;
    int sec = -1;

    const char* rest = string.c_str();

    int iterations = 0;

    if (string.empty() || !result) {
        return false;
    }

    while (*rest) {

        if (iterations++ > 1000) {
            return false;
        }

        switch (*rest) {
        case 'a':
        case 'A':
            if (month == TT_UNKNOWN && (rest[1] == 'p' || rest[1] == 'P') &&
                (rest[2] == 'r' || rest[2] == 'R')) {
                month = TT_APR;
            } else if (zone == TT_UNKNOWN && (rest[1] == 's' || rest[1] == 'S') &&
                       (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_AST;
            } else if (month == TT_UNKNOWN && (rest[1] == 'u' || rest[1] == 'U') &&
                       (rest[2] == 'g' || rest[2] == 'G')) {
                month = TT_AUG;
            }
            break;
        case 'b':
        case 'B':
            if (zone == TT_UNKNOWN && (rest[1] == 's' || rest[1] == 'S') &&
                (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_BST;
            }
            break;
        case 'c':
        case 'C':
            if (zone == TT_UNKNOWN && (rest[1] == 'd' || rest[1] == 'D') &&
                (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_CDT;
            } else if (zone == TT_UNKNOWN && (rest[1] == 's' || rest[1] == 'S') &&
                       (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_CST;
            }
            break;
        case 'd':
        case 'D':
            if (month == TT_UNKNOWN && (rest[1] == 'e' || rest[1] == 'E') &&
                (rest[2] == 'c' || rest[2] == 'C')) {
                month = TT_DEC;
            }
            break;
        case 'e':
        case 'E':
            if (zone == TT_UNKNOWN && (rest[1] == 'd' || rest[1] == 'D') &&
                (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_EDT;
            } else if (zone == TT_UNKNOWN && (rest[1] == 'e' || rest[1] == 'E') &&
                       (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_EET;
            } else if (zone == TT_UNKNOWN && (rest[1] == 's' || rest[1] == 'S') &&
                       (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_EST;
            }
            break;
        case 'f':
        case 'F':
            if (month == TT_UNKNOWN && (rest[1] == 'e' || rest[1] == 'E') &&
                (rest[2] == 'b' || rest[2] == 'B')) {
                month = TT_FEB;
            } else if (dotw == TT_UNKNOWN && (rest[1] == 'r' || rest[1] == 'R') &&
                       (rest[2] == 'i' || rest[2] == 'I')) {
                dotw = TT_FRI;
            }
            break;
        case 'g':
        case 'G':
            if (zone == TT_UNKNOWN && (rest[1] == 'm' || rest[1] == 'M') &&
                (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_GMT;
            }
            break;
        case 'j':
        case 'J':
            if (month == TT_UNKNOWN && (rest[1] == 'a' || rest[1] == 'A') &&
                (rest[2] == 'n' || rest[2] == 'N')) {
                month = TT_JAN;
            } else if (zone == TT_UNKNOWN && (rest[1] == 's' || rest[1] == 'S') &&
                       (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_JST;
            } else if (month == TT_UNKNOWN && (rest[1] == 'u' || rest[1] == 'U') &&
                       (rest[2] == 'l' || rest[2] == 'L')) {
                month = TT_JUL;
            } else if (month == TT_UNKNOWN && (rest[1] == 'u' || rest[1] == 'U') &&
                       (rest[2] == 'n' || rest[2] == 'N')) {
                month = TT_JUN;
            }
            break;
        case 'm':
        case 'M':
            if (month == TT_UNKNOWN && (rest[1] == 'a' || rest[1] == 'A') &&
                (rest[2] == 'r' || rest[2] == 'R')) {
                month = TT_MAR;
            } else if (month == TT_UNKNOWN && (rest[1] == 'a' || rest[1] == 'A') &&
                       (rest[2] == 'y' || rest[2] == 'Y')) {
                month = TT_MAY;
            } else if (zone == TT_UNKNOWN && (rest[1] == 'd' || rest[1] == 'D') &&
                       (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_MDT;
            } else if (zone == TT_UNKNOWN && (rest[1] == 'e' || rest[1] == 'E') &&
                       (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_MET;
            } else if (dotw == TT_UNKNOWN && (rest[1] == 'o' || rest[1] == 'O') &&
                       (rest[2] == 'n' || rest[2] == 'N')) {
                dotw = TT_MON;
            } else if (zone == TT_UNKNOWN && (rest[1] == 's' || rest[1] == 'S') &&
                       (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_MST;
            }
            break;
        case 'n':
        case 'N':
            if (month == TT_UNKNOWN && (rest[1] == 'o' || rest[1] == 'O') &&
                (rest[2] == 'v' || rest[2] == 'V')) {
                month = TT_NOV;
            } else if (zone == TT_UNKNOWN && (rest[1] == 's' || rest[1] == 'S') &&
                       (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_NST;
            }
            break;
        case 'o':
        case 'O':
            if (month == TT_UNKNOWN && (rest[1] == 'c' || rest[1] == 'C') &&
                (rest[2] == 't' || rest[2] == 'T')) {
                month = TT_OCT;
            }
            break;
        case 'p':
        case 'P':
            if (zone == TT_UNKNOWN && (rest[1] == 'd' || rest[1] == 'D') &&
                (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_PDT;
            } else if (zone == TT_UNKNOWN && (rest[1] == 's' || rest[1] == 'S') &&
                       (rest[2] == 't' || rest[2] == 'T')) {
                zone = TT_PST;
            }
            break;
        case 's':
        case 'S':
            if (dotw == TT_UNKNOWN && (rest[1] == 'a' || rest[1] == 'A') &&
                (rest[2] == 't' || rest[2] == 'T')) {
                dotw = TT_SAT;
            } else if (month == TT_UNKNOWN && (rest[1] == 'e' || rest[1] == 'E') &&
                       (rest[2] == 'p' || rest[2] == 'P')) {
                month = TT_SEP;
            } else if (dotw == TT_UNKNOWN && (rest[1] == 'u' || rest[1] == 'U') &&
                       (rest[2] == 'n' || rest[2] == 'N')) {
                dotw = TT_SUN;
            }
            break;
        case 't':
        case 'T':
            if (dotw == TT_UNKNOWN && (rest[1] == 'h' || rest[1] == 'H') &&
                (rest[2] == 'u' || rest[2] == 'U')) {
                dotw = TT_THU;
            } else if (dotw == TT_UNKNOWN && (rest[1] == 'u' || rest[1] == 'U') &&
                       (rest[2] == 'e' || rest[2] == 'E')) {
                dotw = TT_TUE;
            }
            break;
        case 'u':
        case 'U':
            if (zone == TT_UNKNOWN && (rest[1] == 't' || rest[1] == 'T') &&
                !(rest[2] >= 'A' && rest[2] <= 'Z') && !(rest[2] >= 'a' && rest[2] <= 'z')) {
                // UT is the same as GMT but UTx is not.
                zone = TT_GMT;
            }
            break;
        case 'w':
        case 'W':
            if (dotw == TT_UNKNOWN && (rest[1] == 'e' || rest[1] == 'E') &&
                (rest[2] == 'd' || rest[2] == 'D')) {
                dotw = TT_WED;
            }
            break;

        case '+':
        case '-': {
            const char* end;
            int sign;
            if (zone_offset != -1) {
                // already got one...
                rest++;
                break;
            }
            if (zone != TT_UNKNOWN && zone != TT_GMT) {
                // GMT+0300 is legal, but PST+0300 is not.
                rest++;
                break;
            }

            sign = ((*rest == '+') ? 1 : -1);
            rest++; // move over sign
            end = rest;
            while (*end >= '0' && *end <= '9') {
                end++;
            }
            if (rest == end) { // no digits here
                break;
            }

            if ((end - rest) == 4) {
                // offset in HHMM
                zone_offset = (((((rest[0] - '0') * 10) + (rest[1] - '0')) * 60) +
                               (((rest[2] - '0') * 10) + (rest[3] - '0')));
            } else if ((end - rest) == 2) {
                // offset in hours
                zone_offset = (((rest[0] - '0') * 10) + (rest[1] - '0')) * 60;
            } else if ((end - rest) == 1) {
                // offset in hours
                zone_offset = (rest[0] - '0') * 60;
            } else {
                // 3 or >4
                break;
            }

            zone_offset *= sign;
            zone = TT_GMT;
            break;
        }

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            int tmp_hour = -1;
            int tmp_min = -1;
            int tmp_sec = -1;
            const char* end = rest + 1;
            while (*end >= '0' && *end <= '9') {
                end++;
            }

            // end is now the first character after a range of digits.

            if (*end == ':') {
                if (hour >= 0 && min >= 0) { // already got it
                    break;
                }

                // We have seen "[0-9]+:", so this is probably HH:MM[:SS]
                if ((end - rest) > 2) {
                    // it is [0-9][0-9][0-9]+:
                    break;
                }
                if ((end - rest) == 2) {
                    tmp_hour = ((rest[0] - '0') * 10 + (rest[1] - '0'));
                } else {
                    tmp_hour = (rest[0] - '0');
                }

                // move over the colon, and parse minutes

                rest = ++end;
                while (*end >= '0' && *end <= '9') {
                    end++;
                }

                if (end == rest) {
                    // no digits after first colon?
                    break;
                }
                if ((end - rest) > 2) {
                    // it is [0-9][0-9][0-9]+:
                    break;
                }
                if ((end - rest) == 2) {
                    tmp_min = ((rest[0] - '0') * 10 + (rest[1] - '0'));
                } else {
                    tmp_min = (rest[0] - '0');
                }

                // now go for seconds
                rest = end;
                if (*rest == ':') {
                    rest++;
                }
                end = rest;
                while (*end >= '0' && *end <= '9') {
                    end++;
                }

                if (end == rest) {
                    // no digits after second colon - that's ok.
                    ;
                } else if ((end - rest) > 2) {
                    // it is [0-9][0-9][0-9]+:
                    break;
                }
                if ((end - rest) == 2) {
                    tmp_sec = ((rest[0] - '0') * 10 + (rest[1] - '0'));
                } else {
                    tmp_sec = (rest[0] - '0');
                }

                /*
                 * If we made it here, we've parsed hour and min,
                 * and possibly sec, so it worked as a unit.
                 * skip over whitespace and see if there's an AM or PM
                 * directly following the time.
                 */
                if (tmp_hour <= 12) {
                    const char* s = end;
                    while (*s && (*s == ' ' || *s == '\t')) {
                        s++;
                    }
                    if ((s[0] == 'p' || s[0] == 'P') && (s[1] == 'm' || s[1] == 'M')) {
                        // 10:05pm == 22:05, and 12:05pm == 12:05
                        tmp_hour = (tmp_hour == 12 ? 12 : tmp_hour + 12);
                    } else if (tmp_hour == 12 && (s[0] == 'a' || s[0] == 'A') &&
                               (s[1] == 'm' || s[1] == 'M')) {
                        // 12:05am == 00:05
                        tmp_hour = 0;
                    }
                }

                hour = tmp_hour;
                min = tmp_min;
                sec = tmp_sec;
                rest = end;
                break;
            }
            if ((*end == '/' || *end == '-') && end[1] >= '0' && end[1] <= '9') {
                /*
                 * Perhaps this is 6/16/95, 16/6/95, 6-16-95, or 16-6-95
                 * or even 95-06-05...
                 * ####
                 * But it doesn't handle 1995-06-22.
                 */
                int n1, n2, n3;
                const char* s;

                if (month != TT_UNKNOWN) {
                    // if we saw a month name, this can't be.
                    break;
                }

                s = rest;

                n1 = (*s++ - '0'); // first 1 or 2 digits
                if (*s >= '0' && *s <= '9') {
                    n1 = n1 * 10 + (*s++ - '0');
                }

                if (*s != '/' && *s != '-') { // slash
                    break;
                }
                s++;

                if (*s < '0' || *s > '9') { // second 1 or 2 digits
                    break;
                }
                n2 = (*s++ - '0');
                if (*s >= '0' && *s <= '9') {
                    n2 = n2 * 10 + (*s++ - '0');
                }

                if (*s != '/' && *s != '-') { // slash
                    break;
                }
                s++;

                if (*s < '0' || *s > '9') { // third 1, 2, 4, or 5 digits
                    break;
                }
                n3 = (*s++ - '0');
                if (*s >= '0' && *s <= '9') {
                    n3 = n3 * 10 + (*s++ - '0');
                }

                if (*s >= '0' && *s <= '9') {
                    // optional digits 3, 4, and 5
                    n3 = n3 * 10 + (*s++ - '0');
                    if (*s < '0' || *s > '9') {
                        break;
                    }
                    n3 = n3 * 10 + (*s++ - '0');
                    if (*s >= '0' && *s <= '9') {
                        n3 = n3 * 10 + (*s++ - '0');
                    }
                }

                if ((*s >= '0' && *s <= '9') || // followed by non-alphanum
                    (*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z')) {
                    break;
                }

                /*
                 * Ok, we parsed three 1-2 digit numbers, with / or -
                 * between them.  Now decide what the hell they are
                 * (DD/MM/YY or MM/DD/YY or YY/MM/DD.)
                 */

                if (n1 > 31 || n1 == 0) {
                    // must be YY/MM/DD
                    if (n2 > 12) {
                        break;
                    }
                    if (n3 > 31) {
                        break;
                    }
                    year = n1;
                    if (year < 70) {
                        year += 2000;
                    } else if (year < 100) {
                        year += TM_YEAR_BASE;
                    }
                    month = (TIME_TOKEN)(n2 + ((int)TT_JAN) - 1);
                    date = n3;
                    rest = s;
                    break;
                }

                if (n1 > 12 && n2 > 12) {
                    // illegal
                    rest = s;
                    break;
                }

                if (n3 < 70) {
                    n3 += 2000;
                } else if (n3 < 100) {
                    n3 += TM_YEAR_BASE;
                }

                if (n1 > 12) {
                    // must be DD/MM/YY
                    date = n1;
                    month = (TIME_TOKEN)(n2 + ((int)TT_JAN) - 1);
                    year = n3;
                } else { // assume MM/DD/YY
                    month = (TIME_TOKEN)(n1 + ((int)TT_JAN) - 1);
                    date = n2;
                    year = n3;
                }
                rest = s;
            } else if ((*end >= 'A' && *end <= 'Z') || (*end >= 'a' && *end <= 'z')) {
                // Digits followed by non-punctuation - what's that?
                ;
            } else if ((end - rest) == 5) { // five digits is a year
                year =
                    (year < 0 ? ((rest[0] - '0') * 10000L + (rest[1] - '0') * 1000L +
                                 (rest[2] - '0') * 100L + (rest[3] - '0') * 10L + (rest[4] - '0'))
                              : year);
            } else if ((end - rest) == 4) { // four digits is a year
                year = (year < 0 ? ((rest[0] - '0') * 1000L + (rest[1] - '0') * 100L +
                                    (rest[2] - '0') * 10L + (rest[3] - '0'))
                                 : year);
            } else if ((end - rest) == 2) { /* two digits - date or year */
                int n = ((rest[0] - '0') * 10 + (rest[1] - '0'));
                /* If we don't have a date (day of the month) and we see a number
                 * less than 32, then assume that is the date.
                 *
                 * Otherwise, if we have a date and not a year, assume this is
                 * the year.  If it is less than 70, then assume it refers to the 21st century.  If
                 * it is two digits (>= 70), assume it refers to this century.  Otherwise, assume it
                 * refers to an unambiguous year.
                 *
                 *                       The world will surely end soon.
                 */
                if (date < 0 && n < 32) {
                    date = n;
                } else if (year < 0) {
                    if (n < 70) {
                        year = 2000 + n;
                    } else if (n < 100) {
                        year = TM_YEAR_BASE + n;
                    } else {
                        year = n;
                    }
                }
                // else what the hell is this.
            } else if ((end - rest) == 1) { /* one digit - date */
                date = (date < 0 ? (rest[0] - '0') : date);
            }
            // else, three or more than five digits - what's that?
            break;
        }
        }

        /*
         * Skip to the end of this token, whether we parsed it or not.
         * Tokens are delimited by whitespace, or ,;-/
         * But explicitly not :+-.
         */
        while (*rest && *rest != ' ' && *rest != '\t' && *rest != ',' && *rest != ';' &&
               *rest != '-' && *rest != '+' && *rest != '/' && *rest != '(' && *rest != ')' &&
               *rest != '[' && *rest != ']') {
            rest++;
        }
        // skip over uninteresting chars.
    SKIP_MORE:
        while (*rest &&
               (*rest == ' ' || *rest == '\t' || *rest == ',' || *rest == ';' || *rest == '/' ||
                *rest == '(' || *rest == ')' || *rest == '[' || *rest == ']')) {
            rest++;
        }

        /*
         * "-" is ignored at the beginning of a token if we have not yet
         * parsed a year (e.g., the second "-" in "30-AUG-1966"), or if
         * the character after the dash is not a digit.
         */
        if (*rest == '-' &&
            ((isalpha((unsigned char)rest[-1]) && year < 0) || rest[1] < '0' || rest[1] > '9')) {
            rest++;
            goto SKIP_MORE;
        }
    }

    if (zone != TT_UNKNOWN && zone_offset == -1) {
        switch (zone) {
        case TT_PST:
            zone_offset = -8 * 60;
            zone_name = "PST";
            break;
        case TT_PDT:
            zone_offset = -8 * 60;
            dst_offset = 1 * 60;
            zone_name = "PDT";
            break;
        case TT_MST:
            zone_offset = -7 * 60;
            zone_name = "MST";
            break;
        case TT_MDT:
            zone_offset = -7 * 60;
            dst_offset = 1 * 60;
            zone_name = "MDT";
            break;
        case TT_CST:
            zone_offset = -6 * 60;
            zone_name = "CST";
            break;
        case TT_CDT:
            zone_offset = -6 * 60;
            dst_offset = 1 * 60;
            zone_name = "CDT";
            break;
        case TT_EST:
            zone_offset = -5 * 60;
            zone_name = "EST";
            break;
        case TT_EDT:
            zone_offset = -5 * 60;
            dst_offset = 1 * 60;
            zone_name = "EDT";
            break;
        case TT_AST:
            zone_offset = -4 * 60;
            zone_name = "AST";
            break;
        case TT_NST:
            zone_offset = -3 * 60 - 30;
            zone_name = "NST";
            break;
        case TT_GMT:
            zone_offset = 0;
            zone_name = "GMT";
            break;
        case TT_BST:
            zone_offset = 0;
            dst_offset = 1 * 60;
            zone_name = "BST";
            break;
        case TT_MET:
            zone_offset = 1 * 60;
            zone_name = "MET";
            break;
        case TT_EET:
            zone_offset = 2 * 60;
            zone_name = "EET";
            break;
        case TT_JST:
            zone_offset = 9 * 60;
            zone_name = "JST";
            break;
        default:
            break;
        }
    }

    /*
     *  If we didn't find a year, month, or day-of-the-month, we can't
     *  possibly parse this, and in fact, mktime() will do something random
     */
    if (month == TT_UNKNOWN || date == -1 || year == -1 || year > 32767) {
        return false;
    }

    memset(result, 0, sizeof(*result));
    if (sec != -1) {
        result->tm_sec = sec;
    }
    if (min != -1) {
        result->tm_min = min;
    }
    if (hour != -1) {
        result->tm_hour = hour;
    }
    if (date != -1) {
        result->tm_mday = date;
    }
    if (month != TT_UNKNOWN) {
        result->tm_mon = (((int)month) - ((int)TT_JAN));
    }
    if (year != -1) {
        result->tm_year = year - TM_YEAR_BASE;
    }
    if (dotw != TT_UNKNOWN) {
        result->tm_wday = (((int)dotw) - ((int)TT_SUN));
    }

    if (zone == TT_UNKNOWN) {
        // No zone was specified, so pretend the zone was GMT.
        zone = TT_GMT;
        zone_offset = 0;
        zone_name = "GMT";
    }

    result->tm_isdst = dst_offset * 60;
    result->tm_gmtoff = zone_offset * 60;
    result->tm_zone = zone_name;

    return true;
}

std::pair<std::string, std::string> Cookie::NextPair(const std::string& text, size_t& position,
                                                     bool is_name_value) {
    // format is one of:
    //    (1)  name
    //    (2)  name = value
    //    (3)  name = quoted-string
    const size_t length = text.size();
    position = AdvanceTo(text, position);

    size_t semi_colon_position = text.find(';', position);
    if (semi_colon_position == std::string::npos)
        semi_colon_position = length; // no ';' means take everything to end of string

    size_t equals_position = text.find('=', position);
    if (equals_position == std::string::npos || equals_position > semi_colon_position) {
        if (is_name_value) {
            //'=' is required for name-value-pair (RFC6265 section 5.2, rule 2)
            return std::make_pair(std::string(), std::string());
        }
        // no '=' means there is an attribute-name but no attribute-value
        equals_position = semi_colon_position;
    }

    std::string first = Common::StripSpaces(text.substr(position, equals_position - position));
    std::string second;
    int second_length = semi_colon_position - equals_position - 1;
    if (second_length > 0)
        second = Common::StripSpaces(text.substr(equals_position + 1, second_length));

    position = semi_colon_position;
    return std::make_pair(first, second);
}

std::vector<Cookie> Cookie::ParseAllCookies(const std::string& cookies_string) {
    std::vector<Cookie> cookies;
    std::vector<std::string> splitter;

    std::istringstream iss(cookies_string);
    splitter.resize(1);

    while (std::getline(iss, *splitter.rbegin(), '\n')) {
        splitter.emplace_back();
    }

    splitter.pop_back();

    for (size_t c = 0; c < splitter.size(); c++) {
        cookies = ParseCookie(splitter.at(c));
    }

    return cookies;
}

std::vector<Cookie> Cookie::ParseCookie(const std::string& cookie_string) {
    std::vector<Cookie> result;
    size_t position = 0;
    const size_t length = cookie_string.size();
    while (position < length) {
        std::pair<std::string, std::string> field = NextPair(cookie_string, position, true);
        if (field.first.empty()) {
            // parsing error
            break;
        }

        Cookie cookie{field.first, field.second};

        position = AdvanceTo(cookie_string, position);

        while (position < length) {
            switch (cookie_string.at(position++)) {
            case ';':

                // new field in the cookie
                field = NextPair(cookie_string, position, false);
                field.first = Common::ToLower(
                    field.first); // everything but the NAME=VALUE is case-insensitive

                if (field.first == "expires") {
                    position -= field.second.size();
                    size_t end;
                    for (end = position; end < length; ++end) {
                        if (IsValueSeparator(cookie_string.at(end))) {
                            break;
                        }
                    }

                    std::string date_string =
                        Common::StripSpaces(cookie_string.substr(position, end - position));
                    position = end;
                    std::tm dt;
                    if (Cookie::ParseTimeString(date_string, &dt)) {
                        cookie.SetExpirationDate(dt);
                    }
                    // if unparsed, ignore the attribute but not the whole cookie (RFC6265
                    // section 5.2.1)
                } else if (field.first == "domain") {
                    std::string raw_domain = field.second;
                    // empty domain should be ignored (RFC6265 section 5.2.3)
                    if (!raw_domain.empty()) {
                        std::string maybe_leading_dot;
                        if (raw_domain.starts_with('.')) {
                            maybe_leading_dot = '.';
                            raw_domain = raw_domain.substr(1);
                        }
                        // IDN domains are required by RFC6265.
                        // This is where we diverge from spec to avoid linking to an
                        // Internationalization library like iconv.
                        // Instead we just validate the domain.
                        const std::regex url_reg(
                            "^(((?!-))(xn--|_)?[a-z0-9-]{0,61}[a-z0-9]{1,1}.)*(xn--)?([a-z0-9][a-"
                            "z0-9-]{0,60}|[a-z0-9-]{1,30}.[a-z]{2,})$");
                        std::smatch match;
                        if (std::regex_match(raw_domain, match, url_reg)) {
                            cookie.SetDomain(maybe_leading_dot + raw_domain);
                        } else {
                            // Validation failed
                            return result;
                        }
                    }
                } else if (field.first == "max-age") {
                    int secs = stoi(field.second);
                    if (secs <= 0) {
                        // earliest representable time (RFC6265 section 5.2.2)
                        cookie.SetExpirationDate(std::tm{});
                    } else {
                        auto now = std::chrono::system_clock::now();
                        auto then =
                            std::chrono::system_clock::to_time_t(now + std::chrono::seconds{secs});
                        std::tm* expire = std::gmtime(&then);
                        cookie.SetExpirationDate(*expire);
                    }
                    // if unparsed, ignore the attribute but not the whole cookie (RFC6265
                    // section 5.2.2)
                } else if (field.first == "path") {
                    if (field.second.starts_with('/')) {
                        cookie.SetPath(field.second);
                    } else {
                        // if the path doesn't start with '/' then set the default path (RFC6265
                        // section 5.2.4)
                        cookie.SetPath("");
                    }
                } else if (field.first == "secure") {
                    cookie.SetSecure(true);
                } else if (field.first == "httponly") {
                    cookie.SetHttpOnly(true);
                } else if (field.first == "samesite") {
                    cookie.SetSameSitePolicy(
                        Cookie::SameSiteFromString(Common::ToLower(field.second)));
                } else {
                    // ignore unknown fields in the cookie (RFC6265 section 5.2, rule 6)
                }

                position = AdvanceTo(cookie_string, position);
            }
        }

        if (!cookie.m_name.empty()) {
            result.push_back(cookie);
        }
    }

    return result;
}

/*
 *  Returns the raw form of this Cookie.
 */
std::string Cookie::ToString(RawForm form) const {
    std::string result;
    if (m_name.empty()) {
        return result; // not a valid cookie
    }

    result = m_name;
    result += '=';
    result += m_value;

    if (form == RawForm::Full) {
        if (sizeof(m_expiration_date) != 0) {
            result += "; expires=";
            char mbstr[32];
            if (std::strftime(mbstr, sizeof(mbstr), "%a, %e-%b-%Y %T %Z", &m_expiration_date)) {
                result += mbstr;
            }
        }
        if (!m_path.empty()) {
            result += "; path=";
            result += m_path;
        }
        if (!m_domain.empty()) {
            result += "; domain=";
            if (m_domain.starts_with('.')) {
                result += '.';
                result += m_domain.substr(1);
            } else {
                result += m_domain;
            }
        }
        if (IsSecure())
            result += "; secure";
        if (IsHttpOnly())
            result += "; HttpOnly";
        if (m_same_site != SameSite::Default) {
            result += "; SameSite=";
            result += Cookie::SameSiteToString(m_same_site);
        }
    }
    return result;
}

} // namespace WebService
