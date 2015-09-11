// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QTreeWidgetItem>

#include "citra_qt/debugger/registers.h"
#include "citra_qt/util/util.h"

#include "core/core.h"
#include "core/arm/arm_interface.h"

RegistersWidget::RegistersWidget(QWidget* parent) : QDockWidget(parent) {
    cpu_regs_ui.setupUi(this);

    tree = cpu_regs_ui.treeWidget;
    tree->addTopLevelItem(core_registers = new QTreeWidgetItem(QStringList(tr("Registers"))));
    tree->addTopLevelItem(vfp_registers = new QTreeWidgetItem(QStringList(tr("VFP Registers"))));
    tree->addTopLevelItem(vfp_system_registers = new QTreeWidgetItem(QStringList(tr("VFP System Registers"))));
    tree->addTopLevelItem(cpsr = new QTreeWidgetItem(QStringList("CPSR")));

    for (int i = 0; i < 16; ++i) {
        QTreeWidgetItem* child = new QTreeWidgetItem(QStringList(QString("R[%1]").arg(i)));
        core_registers->addChild(child);
    }

    for (int i = 0; i < 32; ++i) {
        QTreeWidgetItem* child = new QTreeWidgetItem(QStringList(QString("S[%1]").arg(i)));
        vfp_registers->addChild(child);
    }

    QFont font = GetMonospaceFont();

    CreateCPSRChildren();
    CreateVFPSystemRegisterChildren();

    // Set Registers to display in monospace font
    for (int i = 0; i < core_registers->childCount(); ++i)
        core_registers->child(i)->setFont(1, font);

    for (int i = 0; i < vfp_registers->childCount(); ++i)
        vfp_registers->child(i)->setFont(1, font);

    for (int i = 0; i < vfp_system_registers->childCount(); ++i) {
        vfp_system_registers->child(i)->setFont(1, font);
        for (int x = 0; x < vfp_system_registers->child(i)->childCount(); ++x) {
            vfp_system_registers->child(i)->child(x)->setFont(1, font);
        }
    }
    // Set CSPR to display in monospace font
    cpsr->setFont(1, font);
    for (int i = 0; i < cpsr->childCount(); ++i) {
        cpsr->child(i)->setFont(1, font);
        for (int x = 0; x < cpsr->child(i)->childCount(); ++x) {
            cpsr->child(i)->child(x)->setFont(1, font);
        }
    }
    setEnabled(false);
}

void RegistersWidget::OnDebugModeEntered() {
    ARM_Interface* app_core = Core::g_app_core;

    if (app_core == nullptr)
        return;

    for (int i = 0; i < core_registers->childCount(); ++i)
        core_registers->child(i)->setText(1, QString("0x%1").arg(app_core->GetReg(i), 8, 16, QLatin1Char('0')));

    for (int i = 0; i < vfp_registers->childCount(); ++i)
        vfp_registers->child(i)->setText(1, QString("0x%1").arg(app_core->GetVFPReg(i), 8, 16, QLatin1Char('0')));

    UpdateCPSRValues();
    UpdateVFPSystemRegisterValues();
}

void RegistersWidget::OnDebugModeLeft() {
}

void RegistersWidget::OnEmulationStarting(EmuThread* emu_thread) {
    setEnabled(true);
}

void RegistersWidget::OnEmulationStopping() {
    // Reset widget text
    for (int i = 0; i < core_registers->childCount(); ++i)
        core_registers->child(i)->setText(1, QString(""));

    for (int i = 0; i < vfp_registers->childCount(); ++i)
        vfp_registers->child(i)->setText(1, QString(""));

    for (int i = 0; i < cpsr->childCount(); ++i)
        cpsr->child(i)->setText(1, QString(""));

    cpsr->setText(1, QString(""));

    // FPSCR
    for (int i = 0; i < vfp_system_registers->child(0)->childCount(); ++i)
        vfp_system_registers->child(0)->child(i)->setText(1, QString(""));

    // FPEXC
    for (int i = 0; i < vfp_system_registers->child(1)->childCount(); ++i)
        vfp_system_registers->child(1)->child(i)->setText(1, QString(""));

    vfp_system_registers->child(0)->setText(1, QString(""));
    vfp_system_registers->child(1)->setText(1, QString(""));
    vfp_system_registers->child(2)->setText(1, QString(""));
    vfp_system_registers->child(3)->setText(1, QString(""));

    setEnabled(false);
}

void RegistersWidget::CreateCPSRChildren() {
    cpsr->addChild(new QTreeWidgetItem(QStringList("M")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("T")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("F")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("I")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("A")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("E")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("IT")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("GE")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("DNM")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("J")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("Q")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("V")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("C")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("Z")));
    cpsr->addChild(new QTreeWidgetItem(QStringList("N")));
}

void RegistersWidget::UpdateCPSRValues() {
    const u32 cpsr_val = Core::g_app_core->GetCPSR();

    cpsr->setText(1, QString("0x%1").arg(cpsr_val, 8, 16, QLatin1Char('0')));
    cpsr->child(0)->setText(1, QString("b%1").arg(cpsr_val & 0x1F, 5, 2, QLatin1Char('0'))); // M - Mode
    cpsr->child(1)->setText(1, QString::number((cpsr_val >> 5) & 1));     // T - State
    cpsr->child(2)->setText(1, QString::number((cpsr_val >> 6) & 1));     // F - FIQ disable
    cpsr->child(3)->setText(1, QString::number((cpsr_val >> 7) & 1));     // I - IRQ disable
    cpsr->child(4)->setText(1, QString::number((cpsr_val >> 8) & 1));     // A - Imprecise abort
    cpsr->child(5)->setText(1, QString::number((cpsr_val >> 9) & 1));     // E - Data endianess
    cpsr->child(6)->setText(1, QString::number((cpsr_val >> 10) & 0x3F)); // IT - If-Then state (DNM)
    cpsr->child(7)->setText(1, QString::number((cpsr_val >> 16) & 0xF));  // GE - Greater-than-or-Equal
    cpsr->child(8)->setText(1, QString::number((cpsr_val >> 20) & 0xF));  // DNM - Do not modify
    cpsr->child(9)->setText(1, QString::number((cpsr_val >> 24) & 1));    // J - Jazelle
    cpsr->child(10)->setText(1, QString::number((cpsr_val >> 27) & 1));   // Q - Saturation
    cpsr->child(11)->setText(1, QString::number((cpsr_val >> 28) & 1));   // V - Overflow
    cpsr->child(12)->setText(1, QString::number((cpsr_val >> 29) & 1));   // C - Carry/Borrow/Extend
    cpsr->child(13)->setText(1, QString::number((cpsr_val >> 30) & 1));   // Z - Zero
    cpsr->child(14)->setText(1, QString::number((cpsr_val >> 31) & 1));   // N - Negative/Less than
}

void RegistersWidget::CreateVFPSystemRegisterChildren() {
    QTreeWidgetItem* const fpscr = new QTreeWidgetItem(QStringList("FPSCR"));
    fpscr->addChild(new QTreeWidgetItem(QStringList("IOC")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("DZC")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("OFC")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("UFC")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("IXC")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("IDC")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("IOE")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("DZE")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("OFE")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("UFE")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("IXE")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("IDE")));
    fpscr->addChild(new QTreeWidgetItem(QStringList(tr("Vector Length"))));
    fpscr->addChild(new QTreeWidgetItem(QStringList(tr("Vector Stride"))));
    fpscr->addChild(new QTreeWidgetItem(QStringList(tr("Rounding Mode"))));
    fpscr->addChild(new QTreeWidgetItem(QStringList("FZ")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("DN")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("V")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("C")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("Z")));
    fpscr->addChild(new QTreeWidgetItem(QStringList("N")));

    QTreeWidgetItem* const fpexc = new QTreeWidgetItem(QStringList("FPEXC"));
    fpexc->addChild(new QTreeWidgetItem(QStringList("IOC")));
    fpexc->addChild(new QTreeWidgetItem(QStringList("OFC")));
    fpexc->addChild(new QTreeWidgetItem(QStringList("UFC")));
    fpexc->addChild(new QTreeWidgetItem(QStringList("INV")));
    fpexc->addChild(new QTreeWidgetItem(QStringList(tr("Vector Iteration Count"))));
    fpexc->addChild(new QTreeWidgetItem(QStringList("FP2V")));
    fpexc->addChild(new QTreeWidgetItem(QStringList("EN")));
    fpexc->addChild(new QTreeWidgetItem(QStringList("EX")));

    vfp_system_registers->addChild(fpscr);
    vfp_system_registers->addChild(fpexc);
    vfp_system_registers->addChild(new QTreeWidgetItem(QStringList("FPINST")));
    vfp_system_registers->addChild(new QTreeWidgetItem(QStringList("FPINST2")));
}

void RegistersWidget::UpdateVFPSystemRegisterValues() {
    const u32 fpscr_val   = Core::g_app_core->GetVFPSystemReg(VFP_FPSCR);
    const u32 fpexc_val   = Core::g_app_core->GetVFPSystemReg(VFP_FPEXC);
    const u32 fpinst_val  = Core::g_app_core->GetVFPSystemReg(VFP_FPINST);
    const u32 fpinst2_val = Core::g_app_core->GetVFPSystemReg(VFP_FPINST2);

    QTreeWidgetItem* const fpscr = vfp_system_registers->child(0);
    fpscr->setText(1, QString("0x%1").arg(fpscr_val, 8, 16, QLatin1Char('0')));
    fpscr->child(0)->setText(1, QString::number(fpscr_val & 1));
    fpscr->child(1)->setText(1, QString::number((fpscr_val >> 1) & 1));
    fpscr->child(2)->setText(1, QString::number((fpscr_val >> 2) & 1));
    fpscr->child(3)->setText(1, QString::number((fpscr_val >> 3) & 1));
    fpscr->child(4)->setText(1, QString::number((fpscr_val >> 4) & 1));
    fpscr->child(5)->setText(1, QString::number((fpscr_val >> 7) & 1));
    fpscr->child(6)->setText(1, QString::number((fpscr_val >> 8) & 1));
    fpscr->child(7)->setText(1, QString::number((fpscr_val >> 9) & 1));
    fpscr->child(8)->setText(1, QString::number((fpscr_val >> 10) & 1));
    fpscr->child(9)->setText(1, QString::number((fpscr_val >> 11) & 1));
    fpscr->child(10)->setText(1, QString::number((fpscr_val >> 12) & 1));
    fpscr->child(11)->setText(1, QString::number((fpscr_val >> 15) & 1));
    fpscr->child(12)->setText(1, QString("b%1").arg((fpscr_val >> 16) & 7, 3, 2, QLatin1Char('0')));
    fpscr->child(13)->setText(1, QString("b%1").arg((fpscr_val >> 20) & 3, 2, 2, QLatin1Char('0')));
    fpscr->child(14)->setText(1, QString("b%1").arg((fpscr_val >> 22) & 3, 2, 2, QLatin1Char('0')));
    fpscr->child(15)->setText(1, QString::number((fpscr_val >> 24) & 1));
    fpscr->child(16)->setText(1, QString::number((fpscr_val >> 25) & 1));
    fpscr->child(17)->setText(1, QString::number((fpscr_val >> 28) & 1));
    fpscr->child(18)->setText(1, QString::number((fpscr_val >> 29) & 1));
    fpscr->child(19)->setText(1, QString::number((fpscr_val >> 30) & 1));
    fpscr->child(20)->setText(1, QString::number((fpscr_val >> 31) & 1));

    QTreeWidgetItem* const fpexc = vfp_system_registers->child(1);
    fpexc->setText(1, QString("0x%1").arg(fpexc_val, 8, 16, QLatin1Char('0')));
    fpexc->child(0)->setText(1, QString::number(fpexc_val & 1));
    fpexc->child(1)->setText(1, QString::number((fpexc_val >> 2) & 1));
    fpexc->child(2)->setText(1, QString::number((fpexc_val >> 3) & 1));
    fpexc->child(3)->setText(1, QString::number((fpexc_val >> 7) & 1));
    fpexc->child(4)->setText(1, QString("b%1").arg((fpexc_val >> 8) & 7, 3, 2, QLatin1Char('0')));
    fpexc->child(5)->setText(1, QString::number((fpexc_val >> 28) & 1));
    fpexc->child(6)->setText(1, QString::number((fpexc_val >> 30) & 1));
    fpexc->child(7)->setText(1, QString::number((fpexc_val >> 31) & 1));

    vfp_system_registers->child(2)->setText(1, QString("0x%1").arg(fpinst_val, 8, 16, QLatin1Char('0')));
    vfp_system_registers->child(3)->setText(1, QString("0x%1").arg(fpinst2_val, 8, 16, QLatin1Char('0')));
}
