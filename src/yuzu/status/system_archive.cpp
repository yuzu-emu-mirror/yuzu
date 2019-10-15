// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QLabel>
#include "core/file_sys/nca_metadata.h"
#include "core/file_sys/romfs_factory.h"
#include "core/file_sys/system_archive/importer.h"
#include "core/file_sys/system_archive/system_archive.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "ui_system_archive.h"
#include "yuzu/status/system_archive.h"

namespace {

enum class Source {
    NAND,
    XCI,
    OSS,
    None,
};

constexpr std::array<const char*, 4> SOURCE_FORMAT = {
    "<html><head/><body><p><span style=\"color:#00aa00;\"><b>8{:02X}</b></span></p></body></html>",
    "<html><head/><body><p><span style=\"color:#0055ff;\"><b>8{:02X}</b></span></p></body></html>",
    "<html><head/><body><p><span style=\"color:#aa00ff;\"><b>8{:02X}</b></span></p></body></html>",
    "<html><head/><body><p><span style=\"color:#aa0000;\"><b>8{:02X}</b></span></p></body></html>",
};

QWidget* CreateItemForSourceAndId(QWidget* parent, Source source, std::size_t id) {
    const auto text = fmt::format(SOURCE_FORMAT.at(static_cast<std::size_t>(source)), id);
    auto* out = new QLabel(QString::fromStdString(text), parent);
    out->setAlignment(Qt::AlignHCenter);
    return out;
}

QWidget* CreateItem(QWidget* parent, const Service::FileSystem::FileSystemController& fsc,
                    std::size_t suffix) {
    const auto title_id = FileSys::SystemArchive::SYSTEM_ARCHIVE_BASE_TITLE_ID + suffix;

    const auto nand =
        fsc.OpenRomFS(title_id, FileSys::StorageId::NandSystem, FileSys::ContentRecordType::Data);

    if (nand.Succeeded()) {
        return CreateItemForSourceAndId(parent, Source::NAND, suffix);
    }

    auto archive = FileSys::SystemArchive::GetImportedSystemArchive(
        fsc.GetSysdataImportedDirectory(), title_id);

    if (archive != nullptr) {
        return CreateItemForSourceAndId(parent, Source::XCI, suffix);
    }

    archive = FileSys::SystemArchive::SynthesizeSystemArchive(title_id);

    if (archive != nullptr) {
        return CreateItemForSourceAndId(parent, Source::OSS, suffix);
    }

    return CreateItemForSourceAndId(parent, Source::None, suffix);
}

} // Anonymous namespace

SystemArchiveDialog::SystemArchiveDialog(QWidget* parent,
                                         const Service::FileSystem::FileSystemController& fsc)
    : QDialog(parent), ui(new Ui::SystemArchiveDialog) {
    ui->setupUi(this);

    for (std::size_t i = 0; i < FileSys::SystemArchive::SYSTEM_ARCHIVE_COUNT; ++i) {
        ui->grid->addWidget(CreateItem(this, fsc, i), i / 6, i % 6, 1, 1);
    }
}

SystemArchiveDialog::~SystemArchiveDialog() = default;
