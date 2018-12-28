// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

class HotkeyRegistry;

namespace Ui {
class ConfigureGeneral;
}

class ConfigureGeneral : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureGeneral(QWidget* parent = nullptr);
    ~ConfigureGeneral() override;

    void ApplyConfiguration();
    void retranslateUi();

private slots:
    void onLanguageChanged(int index);

signals:
    void languageChanged(const QString& locale);

private:
    void SetConfiguration();

    std::unique_ptr<Ui::ConfigureGeneral> ui;
};
