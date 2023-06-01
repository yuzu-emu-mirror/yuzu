// SPDX-FileCopyrightText: 2023 Yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <ctime>
#include <regex>
#include <string>
#include <utility>
#include <vector>

namespace WebService {

class Cookie {
public:
    explicit Cookie(std::string name, std::string value);
    Cookie() = default;
    ~Cookie() = default;

    enum class RawForm { NameAndValueOnly, Full };
    enum class SameSite { Default, None, Lax, Strict };

    static constexpr bool IsTerminator(char c) {
        return c == '\n' || c == '\r';
    }

    static constexpr bool IsValueSeparator(char c) {
        return IsTerminator(c) || c == ';';
    }

    static constexpr bool IsWS(char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    static size_t AdvanceTo(std::string_view text, size_t from) {
        // RFC 2616 defines linear whitespace as:
        //  LWS = [CRLF] 1*( SP | HT )
        // We ignore the fact that CRLF must come as a pair at this point
        // It's an invalid HTTP header if that happens.
        while (from < text.size()) {
            if (IsWS(text[from]))
                ++from;
            else
                return from; // non-whitespace
        }

        // reached the end
        return text.size();
    }

    bool IsSecure() const;
    void SetSecure(bool enable);
    bool IsHttpOnly() const;
    void SetHttpOnly(bool enable);
    SameSite SameSitePolicy() const;
    void SetSameSitePolicy(SameSite same_site);

    std::tm ExpirationDate() const;
    void SetExpirationDate(std::tm date);

    std::string Domain() const;
    void SetDomain(std::string domain);

    std::string Path() const;
    void SetPath(std::string path);

    std::string Name() const;
    void SetName(std::string cookieName);

    std::string Value() const;
    void SetValue(std::string value);

    std::string ToString(RawForm form = RawForm::Full) const;

    static bool ParseTimeString(const std::string& string, std::tm* result);

    static Cookie::SameSite SameSiteFromString(std::string_view str);
    static std::string SameSiteToString(SameSite same_site);

    static std::pair<std::string, std::string> NextPair(const std::string& text, size_t& position,
                                                        bool is_name_value);

    static std::vector<Cookie> ParseAllCookies(const std::string& cookie_string);
    static std::vector<Cookie> ParseCookie(const std::string& cookie_string);

private:
    std::tm m_expiration_date{};
    std::string m_domain;
    std::string m_path;
    std::string m_comment;
    std::string m_name;
    std::string m_value;
    SameSite m_same_site = SameSite::Default;
    bool m_secure = false;
    bool m_http_only = false;
};

} // namespace WebService
