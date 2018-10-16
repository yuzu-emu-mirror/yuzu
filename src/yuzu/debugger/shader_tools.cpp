// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>

#include <QAction>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QTreeView>

#include "core/core.h"
#include "video_core/debug_utils/debug_utils.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/gpu.h"
#include "yuzu/debugger/shader_tools.h"

ShaderToolsDialog::ShaderToolsDialog(std::shared_ptr<Tegra::DebugContext> debug_context,
                                     QWidget* parent)
    : context(debug_context),
      QWidget(parent, Qt::Dialog), Tegra::DebugContext::BreakPointObserver(debug_context) {

    setObjectName("ShaderTools");
    setWindowTitle(tr("Shader Tools"));
    resize(1024, 600);
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);

    layout = new QHBoxLayout;
    item_model = new QStandardItemModel;
    tree_view = new QTreeView;
    code_editor = new QTextEdit;

    tree_view->setAlternatingRowColors(true);
    tree_view->setSelectionMode(QHeaderView::SingleSelection);
    tree_view->setSelectionBehavior(QHeaderView::SelectRows);
    tree_view->setVerticalScrollMode(QHeaderView::ScrollPerPixel);
    tree_view->setHorizontalScrollMode(QHeaderView::ScrollPerPixel);
    tree_view->setSortingEnabled(true);
    tree_view->setEditTriggers(QHeaderView::NoEditTriggers);
    tree_view->setUniformRowHeights(true);
    tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_view->setModel(item_model);
    connect(tree_view, &QAbstractItemView::activated, this, &ShaderToolsDialog::onSelectionChanged);

    item_model->insertColumns(0, COLUMN_COUNT);
    item_model->setHeaderData(COLUMN_STAGE, Qt::Horizontal, tr("Stage"));
    item_model->setHeaderData(COLUMN_ADDRESS, Qt::Horizontal, tr("Address"));

    const QFont fixed_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    code_editor->setFont(fixed_font);

    connect(this, &ShaderToolsDialog::BreakPointHit, this, &ShaderToolsDialog::OnBreakPointHit,
            Qt::BlockingQueuedConnection);
    connect(this, &ShaderToolsDialog::Resumed, this, &ShaderToolsDialog::OnResumed);

    layout->addWidget(code_editor);
    layout->addWidget(tree_view);
    layout->setStretchFactor(code_editor, 1);
    setLayout(layout);

    DisableEditor();
}

void ShaderToolsDialog::OnMaxwellBreakPointHit(Tegra::DebugContext::Event event, void* data) {
    emit BreakPointHit(event, data);
}

void ShaderToolsDialog::OnMaxwellResume() {
    emit Resumed();
}

void ShaderToolsDialog::OnBreakPointHit(Tegra::DebugContext::Event event, void* data) {
    auto& gpu = Core::System::GetInstance().GPU();
    shaders = gpu.Maxwell3D().GetShaderList();
    modified_shaders.resize(shaders.size());
    if (shaders.empty()) {
        return;
    }

    RemoveShaderList();
    for (const auto& shader : shaders) {
        QList<QStandardItem*> items;
        items.append(new QStandardItem(GetShaderStageName(shader.stage)));
        items.append(new QStandardItem(fmt::format("0x{:08x}", shader.addr).c_str()));

        item_model->appendRow(items);
    }

    RestoreSelectedShader();
}

void ShaderToolsDialog::OnResumed() {
    SaveActiveCode();
    SaveScrollPosition();
    SaveSelectedShader();
    active_shader = {};

    auto& gpu = Core::System::GetInstance().GPU();
    for (std::size_t i = 0; i < shaders.size(); ++i) {
        if (!modified_shaders[i]) {
            continue;
        }
        const auto& shader = shaders[i];
        const auto* code = shader.code.c_str();
        gpu.Maxwell3D().InjectShader(shader.addr, shader.code.size(),
                                     reinterpret_cast<const u8*>(code));
    }

    RemoveShaderList();
    DisableEditor();
}

QAction* ShaderToolsDialog::toggleViewAction() {
    if (toggle_view_action != nullptr) {
        return toggle_view_action;
    }
    toggle_view_action = new QAction(windowTitle(), this);
    toggle_view_action->setCheckable(true);
    toggle_view_action->setChecked(isVisible());
    connect(toggle_view_action, &QAction::toggled, this, &ShaderToolsDialog::setVisible);
    return toggle_view_action;
}

void ShaderToolsDialog::showEvent(QShowEvent* ev) {
    if (toggle_view_action) {
        toggle_view_action->setChecked(isVisible());
    }
    QWidget::showEvent(ev);
}

void ShaderToolsDialog::hideEvent(QHideEvent* ev) {
    if (toggle_view_action) {
        toggle_view_action->setChecked(isVisible());
    }
    QWidget::hideEvent(ev);
}

void ShaderToolsDialog::onSelectionChanged() {
    SaveActiveCode();
    SaveScrollPosition();

    SelectShader(tree_view->currentIndex().row());

    RestoreScrollPosition();
}

void ShaderToolsDialog::SelectShader(int index) {
    active_shader = std::make_optional(index);

    if (static_cast<std::size_t>(*active_shader) >= shaders.size()) {
        QMessageBox message_box;
        message_box.setWindowTitle(tr("Shader Tools"));
        message_box.setText(tr("Tried to select an out of bounds shader"));
        message_box.setIcon(QMessageBox::Critical);
        message_box.exec();
        return;
    }

    auto& shader = shaders[*active_shader];
    code_editor->setText(shader.code.c_str());

    EnableEditor();
    RestoreScrollPosition();
}

void ShaderToolsDialog::DisableEditor() {
    code_editor->setText("");
    code_editor->setDisabled(true);
}

void ShaderToolsDialog::EnableEditor() {
    code_editor->setDisabled(false);
}

void ShaderToolsDialog::RemoveShaderList() {
    item_model->removeRows(0, item_model->rowCount());
}

const char* ShaderToolsDialog::GetActiveCode() const {
    return code_editor->toPlainText().toUtf8().constData();
}

void ShaderToolsDialog::SaveActiveCode() {
    if (!active_shader) {
        return;
    }
    auto& shader = shaders[*active_shader];
    const std::string new_code = GetActiveCode();
    if (shader.code != new_code) {
        shader.code = GetActiveCode();
        modified_shaders[*active_shader] = true;
    }
}

void ShaderToolsDialog::SaveScrollPosition() {
    if (!active_shader) {
        return;
    }
    auto& shader = shaders[*active_shader];
    const int scroll_pos = code_editor->verticalScrollBar()->value();
    scroll_map.insert_or_assign(shader.addr, scroll_pos);
}

void ShaderToolsDialog::RestoreScrollPosition() {
    if (!active_shader) {
        return;
    }
    auto& shader = shaders[*active_shader];
    if (const auto it = scroll_map.find(shader.addr); it != scroll_map.end()) {
        const int scroll_pos = it->second;
        code_editor->verticalScrollBar()->setValue(scroll_pos);
    }
}

void ShaderToolsDialog::SaveSelectedShader() {
    if (!active_shader) {
        return;
    }
    auto& shader = shaders[*active_shader];
    last_shader = std::make_optional(shader.addr);
}

void ShaderToolsDialog::RestoreSelectedShader() {
    if (!last_shader) {
        return;
    }
    const auto it =
        std::find_if(shaders.begin(), shaders.end(), [&](const VideoCore::ShaderInfo& shader) {
            return shader.addr == *last_shader;
        });
    if (it == shaders.end())
        return;

    const auto index = static_cast<int>(std::distance(shaders.begin(), it));
    tree_view->setCurrentIndex(item_model->index(index, 0));
    SelectShader(index);
}

QString ShaderToolsDialog::GetShaderStageName(VideoCore::ShaderStage stage) {
    switch (stage) {
    case VideoCore::ShaderStage::Vertex:
        return tr("Vertex");
    case VideoCore::ShaderStage::TesselationControl:
        return tr("Hull");
    case VideoCore::ShaderStage::TesselationEval:
        return tr("Domain");
    case VideoCore::ShaderStage::Geometry:
        return tr("Geometry");
    case VideoCore::ShaderStage::Fragment:
        return tr("Pixel");
    case VideoCore::ShaderStage::Compute:
        return tr("Compute");
    default:
        return tr("Invalid");
    }
}