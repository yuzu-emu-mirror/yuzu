// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <vector>

#include <QWidget>

#include "common/common_types.h"
#include "video_core/shader_info.h"

class QHBoxLayout;
class QStandardItemModel;
class QTreeView;
class QTextEdit;

class ShaderToolsDialog : public QWidget, Tegra::DebugContext::BreakPointObserver {
    Q_OBJECT

public:
    enum {
        COLUMN_STAGE,
        COLUMN_ADDRESS,
        COLUMN_COUNT,
    };

    explicit ShaderToolsDialog(std::shared_ptr<Tegra::DebugContext> debug_context,
                               QWidget* parent = nullptr);

    void OnMaxwellBreakPointHit(Tegra::DebugContext::Event event, void* data) override;
    void OnMaxwellResume() override;

    /// Returns a QAction that can be used to toggle visibility of this dialog.
    QAction* toggleViewAction();

signals:
    void BreakPointHit(Tegra::DebugContext::Event event, void* data);
    void Resumed();

protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private:
    static QString GetShaderStageName(VideoCore::ShaderStage stage);

    void onSelectionChanged();

    void OnBreakPointHit(Tegra::DebugContext::Event event, void* data);
    void OnResumed();

    void SelectShader(int index);

    void DisableEditor();
    void EnableEditor();
    void RemoveShaderList();

    const char* GetActiveCode() const;
    void SaveActiveCode();

    void SaveScrollPosition();
    void RestoreScrollPosition();

    void SaveSelectedShader();
    void RestoreSelectedShader();

    using ShaderAddress = u64;

    std::shared_ptr<Tegra::DebugContext> context;

    std::vector<VideoCore::ShaderInfo> shaders;
    std::vector<bool> modified_shaders;
    std::optional<int> active_shader;

    // UX cached values
    std::map<ShaderAddress, int> scroll_map;
    std::optional<ShaderAddress> last_shader;

    QAction* toggle_view_action = nullptr;
    QHBoxLayout* layout = nullptr;
    QStandardItemModel* item_model = nullptr;
    QTreeView* tree_view = nullptr;
    QTextEdit* code_editor = nullptr;
};