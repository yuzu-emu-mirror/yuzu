// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>
#include "core/file_sys/vfs.h"

namespace Ui {
class ConfigurePerGameDialog;
}

class ConfigurePerGameDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigurePerGameDialog(QWidget* parent, FileSys::VirtualFile file,
                                    const PerGameValuesChange& change);
    ~ConfigurePerGameDialog();

    PerGameValuesChange applyConfiguration();

private:
    void setConfiguration();

    std::unique_ptr<Ui::ConfigurePerGameDialog> ui;
};
