// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QDesktopServices>
#include <QUrl>
#include "common/file_util.h"
#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_debug.h"
#include "yuzu/configuration/configure_debug.h"
#include "yuzu/debugger/console.h"
#include "yuzu/uisettings.h"

ConfigureDebug::ConfigureDebug(QWidget* parent) : QWidget(parent), ui(new Ui::ConfigureDebug) {
    ui->setupUi(this);
    SetConfiguration();

    connect(ui->open_log_button, &QPushButton::clicked, []() {
        QString path = QString::fromStdString(FileUtil::GetUserPath(FileUtil::UserPath::LogDir));
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
}

ConfigureDebug::~ConfigureDebug() = default;

void ConfigureDebug::SetConfiguration() {
    ui->toggle_gdbstub->setChecked(Settings::base_values.use_gdbstub);
    ui->gdbport_spinbox->setEnabled(Settings::base_values.use_gdbstub);
    ui->gdbport_spinbox->setValue(Settings::base_values.gdbstub_port);
    ui->toggle_console->setEnabled(!Core::System::GetInstance().IsPoweredOn());
    ui->toggle_console->setChecked(UISettings::values.show_console);
    ui->log_filter_edit->setText(QString::fromStdString(Settings::config_values->log_filter));
    ui->homebrew_args_edit->setText(QString::fromStdString(Settings::base_values.program_args));
    ui->reporting_services->setChecked(Settings::base_values.reporting_services);
    ui->quest_flag->setChecked(Settings::base_values.quest_flag);
    ui->disable_cpu_opt->setChecked(Settings::base_values.disable_cpu_opt);
    ui->enable_graphics_debugging->setEnabled(!Core::System::GetInstance().IsPoweredOn());
    ui->enable_graphics_debugging->setChecked(Settings::config_values->renderer_debug);
    ui->disable_macro_jit->setEnabled(!Core::System::GetInstance().IsPoweredOn());
    ui->disable_macro_jit->setChecked(Settings::base_values.disable_macro_jit);
}

void ConfigureDebug::ApplyConfiguration() {
    Settings::base_values.use_gdbstub = ui->toggle_gdbstub->isChecked();
    Settings::base_values.gdbstub_port = ui->gdbport_spinbox->value();
    UISettings::values.show_console = ui->toggle_console->isChecked();
    Settings::config_values->log_filter = ui->log_filter_edit->text().toStdString();
    Settings::base_values.program_args = ui->homebrew_args_edit->text().toStdString();
    Settings::base_values.reporting_services = ui->reporting_services->isChecked();
    Settings::base_values.quest_flag = ui->quest_flag->isChecked();
    Settings::base_values.disable_cpu_opt = ui->disable_cpu_opt->isChecked();
    Settings::config_values->renderer_debug = ui->enable_graphics_debugging->isChecked();
    Settings::base_values.disable_macro_jit = ui->disable_macro_jit->isChecked();
    Debugger::ToggleConsole();
    Log::Filter filter;
    filter.ParseFilterString(Settings::config_values->log_filter);
    Log::SetGlobalFilter(filter);
}

void ConfigureDebug::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureDebug::RetranslateUI() {
    ui->retranslateUi(this);
}
