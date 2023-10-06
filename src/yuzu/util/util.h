// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <QFont>
#include <QString>

/// Returns a QFont object appropriate to use as a monospace font for debugging widgets, etc.
QFont GetMonospaceFont();

/// Convert a size in bytes into a readable format (KiB, MiB, etc.)
QString ReadableByteSize(qulonglong size);

/**
 * Creates a circle pixmap from a specified color
 *
 * @param color The color the pixmap shall have
 *
 * @return QPixmap circle pixmap
 */

QPixmap CreateCirclePixmapFromColor(const QColor& color);

/**
 * Creates a circle pixmap from a specified color
 *
 * @param color The color the pixmap shall have
 *
 * @return QPixmap circle pixmap
 */

bool savePixmapsToICO(const QList<QPixmap>& pixmaps, const QString& path);
