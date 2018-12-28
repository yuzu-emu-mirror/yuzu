// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QDirIterator>
#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_general.h"
#include "yuzu/configuration/configure_general.h"
#include "yuzu/ui_settings.h"

ConfigureGeneral::ConfigureGeneral(QWidget* parent)
    : QWidget(parent), ui(new Ui::ConfigureGeneral) {

    ui->setupUi(this);
    ui->language_combobox->addItem(tr("<System>"), QString(""));
    ui->language_combobox->addItem(tr("English"), QString("en"));
    QDirIterator it(":/languages", QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        QString locale = it.next();
        locale.truncate(locale.lastIndexOf('.'));
        locale.remove(0, locale.lastIndexOf('/') + 1);
        QString lang = QLocale::languageToString(QLocale(locale).language());
        ui->language_combobox->addItem(lang, locale);
    }

    // Unlike other configuration, interface language change need to be reflect on the interface
    // immediately. This is done by passing a signal to the main window, and then retranslating when
    // passing back.
    connect(ui->language_combobox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &ConfigureGeneral::onLanguageChanged);

    for (const auto& theme : UISettings::themes) {
        ui->theme_combobox->addItem(QString::fromUtf8(theme.first),
                                    QString::fromUtf8(theme.second));
    }

    SetConfiguration();

    connect(ui->toggle_deepscan, &QCheckBox::stateChanged, this,
            [] { UISettings::values.is_game_list_reload_pending.exchange(true); });

    ui->use_cpu_jit->setEnabled(!Core::System::GetInstance().IsPoweredOn());
}

ConfigureGeneral::~ConfigureGeneral() = default;

void ConfigureGeneral::SetConfiguration() {
    ui->toggle_deepscan->setChecked(UISettings::values.game_directory_deepscan);
    ui->toggle_check_exit->setChecked(UISettings::values.confirm_before_closing);
    ui->toggle_user_on_boot->setChecked(UISettings::values.select_user_on_boot);
    ui->theme_combobox->setCurrentIndex(ui->theme_combobox->findData(UISettings::values.theme));
    ui->language_combobox->setCurrentIndex(
        ui->language_combobox->findData(UISettings::values.language));
    ui->use_cpu_jit->setChecked(Settings::values.use_cpu_jit);
}

void ConfigureGeneral::ApplyConfiguration() {
    UISettings::values.game_directory_deepscan = ui->toggle_deepscan->isChecked();
    UISettings::values.confirm_before_closing = ui->toggle_check_exit->isChecked();
    UISettings::values.select_user_on_boot = ui->toggle_user_on_boot->isChecked();
    UISettings::values.theme =
        ui->theme_combobox->itemData(ui->theme_combobox->currentIndex()).toString();

    Settings::values.use_cpu_jit = ui->use_cpu_jit->isChecked();
}

void ConfigureGeneral::onLanguageChanged(int index) {
    if (index == -1)
        return;

    emit languageChanged(ui->language_combobox->itemData(index).toString());
}

void ConfigureGeneral::retranslateUi() {
    ui->retranslateUi(this);
}
