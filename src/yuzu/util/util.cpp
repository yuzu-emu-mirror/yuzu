// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cmath>
#include <QPainter>
#include "yuzu/util/util.h"
#include <fstream>

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
#include <Windows.h>

#pragma pack(push, 2)
struct ICONDIR {
    WORD idReserved;
    WORD idType;
    WORD idCount;
};

struct ICONDIRENTRY {
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
    bmih.biBitCount = 32;
    bmih.biCompression = BI_RGB;

    // Create an ICO header
    ICONDIR iconDir;
    iconDir.idReserved = 0;
    iconDir.idType = 1;
    iconDir.idCount = 1;

    // Create an ICONDIRENTRY
    ICONDIRENTRY iconEntry;
    iconEntry.bWidth = sourceImage.width();
    iconEntry.bHeight = sourceImage.height() * 2;
    iconEntry.bColorCount = 0;
    iconEntry.bReserved = 0;
    iconEntry.wPlanes = 1;
    iconEntry.wBitCount = 32;
    iconEntry.dwBytesInRes = sizeof(BITMAPINFOHEADER) + imageSize;
    iconEntry.dwImageOffset = sizeof(ICONDIR) + sizeof(ICONDIRENTRY);

    // Save the icon data to a file
    std::ofstream iconFile(path, std::ios::binary);
    if (iconFile.fail())
        return false;
    iconFile.write(reinterpret_cast<const char*>(&iconDir), sizeof(ICONDIR));
    iconFile.write(reinterpret_cast<const char*>(&iconEntry), sizeof(ICONDIRENTRY));
    iconFile.write(reinterpret_cast<const char*>(&bmih), sizeof(BITMAPINFOHEADER));

    for (int y = 0; y < image.height(); y++) {
        auto line = (char*)sourceImage.scanLine(sourceImage.height() - 1 - y);
        iconFile.write(line, sourceImage.width() * 4);
    }

    iconFile.close();

    return true;
}
#else
bool SaveAsIco(QImage image) {
    return false;
}
#endif