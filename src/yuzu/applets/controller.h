// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <QDialog>
#include "core/frontend/applets/controller.h"

class ConfigureInputSimple;
class GMainWindow;
class QDialogButtonBox;
class QVBoxLayout;

class QtControllerAppletDialog final : public QDialog {
    Q_OBJECT

public:
    QtControllerAppletDialog(QWidget* parent, Core::Frontend::ControllerParameters parameters);
    ~QtControllerAppletDialog() override;

    void accept() override;
    void reject() override;

    bool GetStatus() const;

private:
    bool ok = false;

    QVBoxLayout* layout;

    QDialogButtonBox* buttons;
    ConfigureInputSimple* input;

    Core::Frontend::ControllerParameters parameters;
};

class QtControllerApplet final : public QObject, public Core::Frontend::ControllerApplet {
    Q_OBJECT

public:
    explicit QtControllerApplet(GMainWindow& parent);
    ~QtControllerApplet() override;

    void ReconfigureControllers(std::function<void(bool)> completed,
                                Core::Frontend::ControllerParameters parameters) const override;

signals:
    void MainWindowReconfigureControllers(Core::Frontend::ControllerParameters parameters) const;

private:
    void MainWindowReconfigureFinished(bool ok);

    mutable std::function<void(bool)> completed;
};
