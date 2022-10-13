// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QRegExp>
#include <QString>
#include <QValidator>

class Validation {
public:
    Validation()
        : room_name(room_name_regex), nickname(nickname_regex), ip(ip_regex), port(0, UINT16_MAX) {}

    ~Validation() = default;

    const QValidator* GetRoomName() const {
        return &room_name;
    }

    const QValidator* GetNickname() const {
        return &nickname;
    }

    const QValidator* GetIP() const {
        return &ip;
    }

    const QValidator* GetPort() const {
        return &port;
    }

private:
    /// room name can be alphanumeric and " " "_" "." and "-" and must have a size of 4-20
    QRegExp room_name_regex = QRegExp(QStringLiteral("^[a-zA-Z0-9._- ]{4,20}$"));
    QRegExpValidator room_name;

    /// nickname can be alphanumeric and " " "_" "." and "-" and must have a size of 4-20
    QRegExp nickname_regex = QRegExp(QStringLiteral("^[a-zA-Z0-9._- ]{4,20}$"));
    QRegExpValidator nickname;

    /// Allow ip address and hostname
    QRegExp ip_regex =
        QRegExp(QStringLiteral("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4]"
                               "[0-9]|[01]?[0-9][0-9]?)$|^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*["
                               "a-zA-Z0-9])\.)+([A-Za-z]|[A-Za-z][A-Za-z0-9\-]*[A-Za-z0-9])$"));
    QRegExpValidator ip;

    /// port must be between 0 and 65535
    QIntValidator port;
};
