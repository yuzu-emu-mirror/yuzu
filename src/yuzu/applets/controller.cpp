// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QDialogButtonBox>
#include <QLabel>
#include "core/hle/lock.h"
#include "core/hle/service/hid/hid.h"
#include "yuzu/applets/controller.h"
#include "yuzu/configuration/configure_input_simple.h"
#include "yuzu/main.h"

QtControllerAppletDialog::QtControllerAppletDialog(
    QWidget* parent, Core::Frontend::ControllerParameters parameters) {
    layout = new QVBoxLayout;
    buttons = new QDialogButtonBox;
    buttons->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
    buttons->addButton(tr("OK"), QDialogButtonBox::AcceptRole);

    connect(buttons, &QDialogButtonBox::accepted, this, &QtControllerAppletDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QtControllerAppletDialog::reject);

    input = new ConfigureInputSimple(this, parameters);
    layout->addWidget(input);
    layout->addWidget(buttons);
    setLayout(layout);
}

QtControllerAppletDialog::~QtControllerAppletDialog() = default;

void QtControllerAppletDialog::accept() {
    input->applyConfiguration();
    ok = true;
    QDialog::accept();
}

void QtControllerAppletDialog::reject() {
    ok = false;
    QDialog::reject();
}

bool QtControllerAppletDialog::GetStatus() const {
    return ok;
}

QtControllerApplet::QtControllerApplet(GMainWindow& parent) {
    connect(this, &QtControllerApplet::MainWindowReconfigureControllers, &parent,
            &GMainWindow::ControllerAppletReconfigureControllers, Qt::QueuedConnection);
    connect(&parent, &GMainWindow::ControllerAppletReconfigureFinished, this,
            &QtControllerApplet::MainWindowReconfigureFinished, Qt::QueuedConnection);
}

QtControllerApplet::~QtControllerApplet() = default;

void QtControllerApplet::ReconfigureControllers(
    std::function<void(bool)> completed, Core::Frontend::ControllerParameters parameters) const {
    this->completed = std::move(completed);
    emit MainWindowReconfigureControllers(parameters);
}

void QtControllerApplet::MainWindowReconfigureFinished(bool ok) {
    // Acquire the HLE mutex
    std::lock_guard<std::recursive_mutex> lock(HLE::g_hle_lock);
    completed(ok);
}
