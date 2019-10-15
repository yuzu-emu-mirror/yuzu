// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>

namespace Service::FileSystem {
class FileSystemController;
} // namespace Service::FileSystem

namespace Ui {
class SystemArchiveDialog;
} // namespace Ui

class SystemArchiveDialog : public QDialog {
    Q_OBJECT

public:
    explicit SystemArchiveDialog(QWidget* parent,
                                 const Service::FileSystem::FileSystemController& fsc);
    ~SystemArchiveDialog() override;

private:
    std::unique_ptr<Ui::SystemArchiveDialog> ui;
};
