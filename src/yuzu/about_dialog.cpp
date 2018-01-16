// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/scm_rev.h"
#include "ui_aboutdialog.h"
#include "yuzu/about_dialog.h"

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent), ui(new Ui::AboutDialog) {
    ui->setupUi(this);
    ui->labelBuildInfo->setText(ui->labelBuildInfo->text().arg(
        Common::g_build_name, Common::g_scm_branch, Common::g_scm_desc));
}

AboutDialog::~AboutDialog() {}
