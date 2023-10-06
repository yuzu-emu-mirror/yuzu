// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QTemporaryFile>

#include "yuzu/util/util.h"

QFont GetMonospaceFont() {
    QFont font(QStringLiteral("monospace"));
    // Automatic fallback to a monospace font on on platforms without a font called "monospace"
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    return font;
}

QString ReadableByteSize(qulonglong size) {
    static constexpr std::array units{"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    if (size == 0) {
        return QStringLiteral("0");
    }

    const int digit_groups = std::min(static_cast<int>(std::log10(size) / std::log10(1024)),
                                      static_cast<int>(units.size()));
    return QStringLiteral("%L1 %2")
        .arg(size / std::pow(1024, digit_groups), 0, 'f', 1)
        .arg(QString::fromUtf8(units[digit_groups]));
}

QPixmap CreateCirclePixmapFromColor(const QColor& color) {
    QPixmap circle_pixmap(16, 16);
    circle_pixmap.fill(Qt::transparent);
    QPainter painter(&circle_pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(color);
    painter.setBrush(color);
    painter.drawEllipse({circle_pixmap.width() / 2.0, circle_pixmap.height() / 2.0}, 7.0, 7.0);
    return circle_pixmap;
}

#if defined(WIN32)
template <typename T>
void write(QFile& f, const T t) {
    f.write((const char*)&t, sizeof(t));
}

bool savePixmapsToICO(const QList<QPixmap>& pixmaps, const QString& path) {
    static_assert(sizeof(short) == 2, "short int is not 2 bytes");
    static_assert(sizeof(int) == 4, "int is not 4 bytes");

    QFile f(path);
    if (!f.open(QFile::OpenModeFlag::WriteOnly))
        return false;

    // Header
    write<short>(f, 0);
    write<short>(f, 1);
    write<short>(f, pixmaps.count());

    QList<int> images_size;
    for (int ii = 0; ii < pixmaps.count(); ++ii) {
        QTemporaryFile temp;
        temp.setAutoRemove(true);
        if (!temp.open())
            return false;

        const auto& pixmap = pixmaps[ii];
        pixmap.save(&temp, "PNG");

        temp.close();

        images_size.push_back(QFileInfo(temp).size());
    }

    // Images directory
    constexpr unsigned int entry_size = sizeof(char) + sizeof(char) + sizeof(char) + sizeof(char) +
                                        sizeof(short) + sizeof(short) + sizeof(unsigned int) +
                                        sizeof(unsigned int);
    static_assert(entry_size == 16, "wrong entry size");

    unsigned int offset = 3 * sizeof(short) + pixmaps.count() * entry_size;
    for (int ii = 0; ii < pixmaps.count(); ++ii) {
        const auto& pixmap = pixmaps[ii];
        if (pixmap.width() > 256 || pixmap.height() > 256)
            continue;

        write<char>(f, pixmap.width() == 256 ? 0 : pixmap.width());
        write<char>(f, pixmap.height() == 256 ? 0 : pixmap.height());
        write<char>(f, 0);                       // palette size
        write<char>(f, 0);                       // reserved
        write<short>(f, 1);                      // color planes
        write<short>(f, pixmap.depth());         // bits-per-pixel
        write<unsigned int>(f, images_size[ii]); // size of image in bytes
        write<unsigned int>(f, offset);          // offset
        offset += images_size[ii];
    }

    for (int ii = 0; ii < pixmaps.count(); ++ii) {
        const auto& pixmap = pixmaps[ii];
        if (pixmap.width() > 256 || pixmap.height() > 256)
            continue;
        pixmap.save(&f, "PNG");
    }

    // Close the file before renaming it
    f.close();

    // Remove the .png extension Add the .ico extension and Rename the file
    QString qPath = path;
    qPath.chop(3);
    qPath = qPath % QString::fromStdString("ico");
    QFile::rename(path, qPath);

    return true;
}
#else

bool SaveAsIco(const QList<QPixmap>& pixmaps, const QString& path) {
    return false;
}
#endif