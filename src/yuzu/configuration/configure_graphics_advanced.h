// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

namespace Core {
class System;
}

namespace ConfigurationShared {
enum class CheckState;
}

namespace Ui {
class ConfigureGraphicsAdvanced;
}

class ConfigureGraphicsAdvanced : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureGraphicsAdvanced(const Core::System& system_, QWidget* parent = nullptr);
    ~ConfigureGraphicsAdvanced() override;

    void ApplyConfiguration();
    void SetConfiguration();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    void SetupPerGameUI();

    std::unique_ptr<Ui::ConfigureGraphicsAdvanced> ui;

    ConfigurationShared::CheckState use_vsync;
    ConfigurationShared::CheckState use_asynchronous_shaders;
    ConfigurationShared::CheckState use_fast_gpu_time;
    ConfigurationShared::CheckState use_caches_gc_adv;

    const Core::System& system;
};
