// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QHash>
#include <QListWidgetItem>
#include "core/settings.h"
#include "ui_configure.h"
#include "yuzu/configuration/config.h"
#include "yuzu/configuration/configure_dialog.h"
#include "yuzu/configuration/configure_input_player.h"
#include "yuzu/hotkeys.h"

ConfigureDialog::ConfigureDialog(QWidget* parent, HotkeyRegistry& registry)
    : QDialog(parent), ui(new Ui::ConfigureDialog), registry(registry) {
    ui->setupUi(this);
    ui->hotkeysTab->Populate(registry);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    PopulateSelectionList();

    connect(ui->uiTab, &ConfigureUi::languageChanged, this, &ConfigureDialog::onLanguageChanged);
    connect(ui->selectorList, &QListWidget::itemSelectionChanged, this,
            &ConfigureDialog::UpdateVisibleTabs);

    adjustSize();
    ui->selectorList->setCurrentRow(0);
}

ConfigureDialog::~ConfigureDialog() = default;

void ConfigureDialog::SetConfiguration() {
    ui->generalTab->SetConfiguration();
    ui->uiTab->SetConfiguration();
    ui->systemTab->SetConfiguration();
    ui->inputTab->LoadConfiguration();
    ui->graphicsTab->SetConfiguration();
    ui->audioTab->SetConfiguration();
    ui->debugTab->SetConfiguration();
    ui->webTab->SetConfiguration();
}

void ConfigureDialog::ApplyConfiguration() {
    ui->generalTab->ApplyConfiguration();
    ui->uiTab->ApplyConfiguration();
    ui->systemTab->ApplyConfiguration();
    ui->profileManagerTab->ApplyConfiguration();
    ui->inputTab->ApplyConfiguration();
    ui->hotkeysTab->ApplyConfiguration(registry);
    ui->graphicsTab->ApplyConfiguration();
    ui->audioTab->ApplyConfiguration();
    ui->debugTab->ApplyConfiguration();
    ui->webTab->ApplyConfiguration();
    Settings::Apply();
    Settings::LogSettings();
}

void ConfigureDialog::PopulateSelectionList() {
    ui->selectorList->clear();

    const std::array<std::pair<QString, QStringList>, 4> items{
        {{tr("General"),
          {QT_TR_NOOP(QStringLiteral("General")), QT_TR_NOOP(QStringLiteral("Web")),
           QT_TR_NOOP(QStringLiteral("Debug")), QT_TR_NOOP(QStringLiteral("UI"))}},
         {tr("System"),
          {QT_TR_NOOP(QStringLiteral("System")), QT_TR_NOOP(QStringLiteral("Profiles")),
           QT_TR_NOOP(QStringLiteral("Audio"))}},
         {tr("Graphics"), {QT_TR_NOOP(QStringLiteral("Graphics"))}},
         {tr("Controls"),
          {QT_TR_NOOP(QStringLiteral("Input")), QT_TR_NOOP(QStringLiteral("Hotkeys"))}}}};

    for (const auto& entry : items) {
        auto* const item = new QListWidgetItem(entry.first);
        item->setData(Qt::UserRole, entry.second);

        ui->selectorList->addItem(item);
    }
}

void ConfigureDialog::UpdateVisibleTabs() {
    const auto items = ui->selectorList->selectedItems();
    if (items.isEmpty())
        return;

    const std::map<QString, QWidget*> widgets = {
        {QStringLiteral("General"), ui->generalTab},
        {QStringLiteral("System"), ui->systemTab},
        {QStringLiteral("Profiles"), ui->profileManagerTab},
        {QStringLiteral("Input"), ui->inputTab},
        {QStringLiteral("Hotkeys"), ui->hotkeysTab},
        {QStringLiteral("Graphics"), ui->graphicsTab},
        {QStringLiteral("Audio"), ui->audioTab},
        {QStringLiteral("Debug"), ui->debugTab},
        {QStringLiteral("Web"), ui->webTab},
        {QStringLiteral("UI"), ui->uiTab}};

    ui->tabWidget->clear();

    const QStringList tabs = items[0]->data(Qt::UserRole).toStringList();

    for (const auto& tab : tabs)
        ui->tabWidget->addTab(widgets.find(tab)->second, tr(qPrintable(tab)));
}

void ConfigureDialog::onLanguageChanged(const QString& locale) {
    emit languageChanged(locale);
    // first apply the configuration, and then restore the display
    ApplyConfiguration();
    RetranslateUI();
    SetConfiguration();
}

void ConfigureDialog::RetranslateUI() {
    int old_row = ui->selectorList->currentRow();
    int old_index = ui->tabWidget->currentIndex();
    ui->retranslateUi(this);
    PopulateSelectionList();
    // restore selection after repopulating
    ui->selectorList->setCurrentRow(old_row);
    ui->tabWidget->setCurrentIndex(old_index);

    ui->generalTab->RetranslateUI();
    ui->uiTab->RetranslateUI();
    ui->systemTab->RetranslateUI();
    ui->inputTab->RetranslateUI();
    ui->graphicsTab->RetranslateUI();
    ui->audioTab->RetranslateUI();
    ui->debugTab->RetranslateUI();
    ui->webTab->RetranslateUI();
}
