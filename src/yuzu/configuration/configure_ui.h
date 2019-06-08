// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

namespace Ui {
class ConfigureUi;
}

class ConfigureUi : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureUi(QWidget* parent = nullptr);
    ~ConfigureUi() override;

    void ApplyConfiguration();
    void RetranslateUI();
    void SetConfiguration();

private slots:
    void onLanguageChanged(int index);

signals:
    void languageChanged(const QString& locale);

private:
    void RequestGameListUpdate();

    void changeEvent(QEvent*) override;

    void InitializeLanguageComboBox();
    void InitializeIconSizeComboBox();
    void InitializeRowComboBoxes();

    std::unique_ptr<Ui::ConfigureUi> ui;
};
