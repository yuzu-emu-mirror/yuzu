// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFont>
#include <QImage>
#include <QString>

enum class GameListShortcutTarget;

/// Returns a QFont object appropriate to use as a monospace font for debugging widgets, etc.
[[nodiscard]] QFont GetMonospaceFont();

/// Convert a size in bytes into a readable format (KiB, MiB, etc.)
[[nodiscard]] QString ReadableByteSize(qulonglong size);

/**
 * Creates a circle pixmap from a specified color
 * @param color The color the pixmap shall have
 * @return QPixmap circle pixmap
 */
[[nodiscard]] QPixmap CreateCirclePixmapFromColor(const QColor& color);

/**
 * Returns the path of the provided shortcut target
 * @return QString containing the path to the target
 */
[[nodiscard]] QString GetTargetPath(GameListShortcutTarget target);

/**
 * Saves a windows icon to a file
 * @param file_path The icon file path
 * @param image The image to save
 * @return bool If the operation succeeded
 */
[[nodiscard]] bool SaveIconToFile(const QString file_path, const QImage& image);

/**
 * Creates and saves a shortcut to the shortcut_path
 * @return bool If the operation succeeded
 */
bool CreateShortcut(const std::string& shortcut_path, const std::string& title,
                    const std::string& comment, const std::string& icon_path,
                    const std::string& command, const std::string& arguments,
                    const std::string& categories, const std::string& keywords);
