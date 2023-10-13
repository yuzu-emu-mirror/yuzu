// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cmath>
#include <fstream>
#include <QApplication>
#include <QPainter>
#include <QStandardPaths>

#include "common/fs/file.h"
#include "common/string_util.h"
#include "yuzu/game_list.h"
#include "yuzu/util/util.h"

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
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

QString GetTargetPath(GameListShortcutTarget target) {
    if (target == GameListShortcutTarget::Desktop) {
        return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }

    if (target != GameListShortcutTarget::Applications) {
        return {};
    }

    const QString applications_path =
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);

    if (!applications_path.isEmpty()) {
        return applications_path;
    }

    // If Qt fails to find the application path try to use the generic location
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
}

bool SaveIconToFile(const QString file_path, const QImage& image) {
#if defined(WIN32)
#pragma pack(push, 2)
    struct IconDir {
        WORD id_reserved;
        WORD id_type;
        WORD id_count;
    };

    struct IconDirEntry {
        BYTE width;
        BYTE height;
        BYTE color_count;
        BYTE reserved;
        WORD planes;
        WORD bit_count;
        DWORD bytes_in_res;
        DWORD image_offset;
    };
#pragma pack(pop)

    const QImage source_image = image.convertToFormat(QImage::Format_RGB32);
    constexpr std::array<int, 7> scale_sizes{256, 128, 64, 48, 32, 24, 16};
    constexpr int bytes_per_pixel = 4;

    const IconDir icon_dir{
        .id_reserved = 0,
        .id_type = 1,
        .id_count = static_cast<WORD>(scale_sizes.size()),
    };

    Common::FS::IOFile icon_file(file_path.toUtf8().toStdString(),
                                 Common::FS::FileAccessMode::Write,
                                 Common::FS::FileType::BinaryFile);
    if (!icon_file.IsOpen()) {
        return false;
    }

    if (!icon_file.Write(icon_dir)) {
        return false;
    }

    std::size_t image_offset = sizeof(IconDir) + (sizeof(IconDirEntry) * scale_sizes.size());
    for (std::size_t i = 0; i < scale_sizes.size(); i++) {
        const int image_size = scale_sizes[i] * scale_sizes[i] * bytes_per_pixel;
        const IconDirEntry icon_entry{
            .width = static_cast<BYTE>(scale_sizes[i]),
            .height = static_cast<BYTE>(scale_sizes[i]),
            .color_count = 0,
            .reserved = 0,
            .planes = 1,
            .bit_count = bytes_per_pixel * 8,
            .bytes_in_res = static_cast<DWORD>(sizeof(BITMAPINFOHEADER) + image_size),
            .image_offset = static_cast<DWORD>(image_offset),
        };
        image_offset += icon_entry.bytes_in_res;
        if (!icon_file.Write(icon_entry)) {
            return false;
        }
    }

    for (std::size_t i = 0; i < scale_sizes.size(); i++) {
        const QImage scaled_image = source_image.scaled(
            scale_sizes[i], scale_sizes[i], Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        const BITMAPINFOHEADER info_header{
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = scaled_image.width(),
            .biHeight = scaled_image.height() * 2,
            .biPlanes = 1,
            .biBitCount = bytes_per_pixel * 8,
            .biCompression = BI_RGB,
            .biSizeImage{},
            .biXPelsPerMeter{},
            .biYPelsPerMeter{},
            .biClrUsed{},
            .biClrImportant{},
        };

        if (!icon_file.Write(info_header)) {
            return false;
        }

        for (int y = 0; y < scaled_image.height(); y++) {
            const auto* line = scaled_image.scanLine(scaled_image.height() - 1 - y);
            std::vector<u8> line_data(scaled_image.width() * bytes_per_pixel);
            std::memcpy(line_data.data(), line, line_data.size());
            if (!icon_file.Write(line_data)) {
                return false;
            }
        }
    }
    icon_file.Close();

    return true;
#else
    return image.save(file_path);
#endif
}

bool CreateShortcut(const std::string& shortcut_path, const std::string& title,
                    const std::string& comment, const std::string& icon_path,
                    const std::string& command, const std::string& arguments,
                    const std::string& categories, const std::string& keywords) {
#if defined(__linux__) || defined(__FreeBSD__)
    // This desktop file template was writing referencing
    // https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-1.0.html
    std::string shortcut_contents{};
    shortcut_contents.append("[Desktop Entry]\n");
    shortcut_contents.append("Type=Application\n");
    shortcut_contents.append("Version=1.0\n");
    shortcut_contents.append(fmt::format("Name={:s}\n", title));
    shortcut_contents.append(fmt::format("Comment={:s}\n", comment));
    shortcut_contents.append(fmt::format("Icon={:s}\n", icon_path));
    shortcut_contents.append(fmt::format("TryExec={:s}\n", command));
    shortcut_contents.append(fmt::format("Exec={:s} {:s}\n", command, arguments));
    shortcut_contents.append(fmt::format("Categories={:s}\n", categories));
    shortcut_contents.append(fmt::format("Keywords={:s}\n", keywords));

    std::ofstream shortcut_stream(shortcut_path);
    if (!shortcut_stream.is_open()) {
        LOG_WARNING(Common, "Failed to create file {:s}", shortcut_path);
        return false;
    }
    shortcut_stream << shortcut_contents;
    shortcut_stream.close();

    return true;
#elif defined(WIN32)
    IShellLinkW* shell_link;
    auto hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW,
                                 (void**)&shell_link);
    if (FAILED(hres)) {
        return false;
    }
    shell_link->SetPath(
        Common::UTF8ToUTF16W(command).data()); // Path to the object we are referring to
    shell_link->SetArguments(Common::UTF8ToUTF16W(arguments).data());
    shell_link->SetDescription(Common::UTF8ToUTF16W(comment).data());
    shell_link->SetIconLocation(Common::UTF8ToUTF16W(icon_path).data(), 0);

    IPersistFile* persist_file;
    hres = shell_link->QueryInterface(IID_IPersistFile, (void**)&persist_file);
    if (FAILED(hres)) {
        return false;
    }

    hres = persist_file->Save(Common::UTF8ToUTF16W(shortcut_path).data(), TRUE);
    if (FAILED(hres)) {
        return false;
    }

    persist_file->Release();
    shell_link->Release();

    return true;
#endif
    return false;
}
