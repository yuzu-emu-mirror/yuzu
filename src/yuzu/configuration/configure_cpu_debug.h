// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

namespace Ui {
class ConfigureCpuDebug;
}

class ConfigureCpuDebug : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureCpuDebug(QWidget* parent = nullptr);
    ~ConfigureCpuDebug() override;

    void ApplyConfiguration();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    void SetConfiguration();

    std::unique_ptr<Ui::ConfigureCpuDebug> ui;
};
