// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cmath>
#include <QPainter>
#include "yuzu/util/util.h"
#if defined(WIN32)
#include <fstream>
#include <Windows.h>
#endif

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
#pragma pack(push, 2)
struct IconDir {
    WORD id_reserved;
    WORD id_type;
    WORD id_count;
};

struct IconDirEntry {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    DWORD dwImageOffset;
};
#pragma pack(pop)

bool SaveIconToFile(const char* path, QImage image) {

    QImage sourceImage = image.convertToFormat(QImage::Format_RGB32);

    const int bytesPerPixel = 4;
    const int imageSize = sourceImage.width() * sourceImage.height() * bytesPerPixel;

    BITMAPINFOHEADER bmih = {};
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = sourceImage.width();
    bmih.biHeight = sourceImage.height() * 2;
    bmih.biPlanes = 1;
    bmih.biBitCount = bytesPerPixel * 8;
    bmih.biCompression = BI_RGB;

    // Create an ICO header
    IconDir iconDir;
    iconDir.id_reserved = 0;
    iconDir.id_type = 1;
    iconDir.id_count = 1;

    // Create an ICONDIRENTRY
    IconDirEntry iconEntry;
    iconEntry.bWidth = sourceImage.width();
    iconEntry.bHeight = sourceImage.height() * 2;
    iconEntry.bColorCount = 0;
    iconEntry.bReserved = 0;
    iconEntry.wPlanes = 1;
    iconEntry.wBitCount = bytesPerPixel * 8;
    iconEntry.dwBytesInRes = sizeof(BITMAPINFOHEADER) + imageSize;
    iconEntry.dwImageOffset = sizeof(IconDir) + sizeof(IconDirEntry);

    // Save the icon data to a file
    std::ofstream iconFile(path, std::ios::binary);
    if (iconFile.fail())
        return false;
    iconFile.write(reinterpret_cast<const char*>(&iconDir), sizeof(IconDir));
    iconFile.write(reinterpret_cast<const char*>(&iconEntry), sizeof(IconDirEntry));
    iconFile.write(reinterpret_cast<const char*>(&bmih), sizeof(BITMAPINFOHEADER));

    for (int y = 0; y < image.height(); y++) {
        auto line = reinterpret_cast<const char*>(sourceImage.scanLine(sourceImage.height() - 1 - y));
        iconFile.write(line, sourceImage.width() * bytesPerPixel);
    }

    iconFile.close();

    return true;
}
#else
bool SaveAsIco(QImage image) {
    return false;
}
#endif
