/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "DocumentEditor.h"
#include "LadderModel.h"

class LadderCanvasView;
class LadderInstructionBar;
class LadderPropertiesPanel;
class LadderScene;
class LadderElementItem;

class LadderEditorWidget : public DocumentEditor
{
    Q_OBJECT

public:
    explicit LadderEditorWidget(QWidget *parent = nullptr);

    LadderScene *scene() const { return m_scene; }
    LadderPropertiesPanel *propertiesPanel() const { return m_propertiesPanel; }
    LadderInstructionBar *instructionBar() const { return m_instructionBar; }
    QString exportedStSource() const { return m_lastExportedSt; }

    bool loadFromPath(const QString &path) override;
    bool saveToPath(const QString &path) override;
    bool isDirty() const override { return m_dirty; }
    void markClean() override;
    void newDocument() override;
    QString suggestedSaveFilter() const override;
    QString suggestedOpenFilter() const override;
    QString compileInputPath(QString *tempCleanupPath) override;

    void setUseNativeXmlCompile(bool enabled);

public slots:
    void showGeneratedSt();
    void showVariableBrowser();
    bool importFromStSource(const QString &source, QStringList *diagnostics = nullptr);
    bool importFromStPath(const QString &path, QStringList *diagnostics = nullptr);
    void applyImportedProgram(const LadderProgram &program);

signals:
    void selectionChanged(LadderElementItem *item);

private:
    void rebuildUi();
    void onProgramModified();
    void updateSelection();
    void handleDoubleClick(LadderElementItem *item);
    void refreshElementPresentation(int localId, bool reselect = true);
    bool exportCompileSource(QString *errorMessage);
    bool exportCompileXml(QString *errorMessage);

    LadderModel m_model;
    LadderScene *m_scene = nullptr;
    LadderCanvasView *m_view = nullptr;
    LadderInstructionBar *m_instructionBar = nullptr;
    LadderPropertiesPanel *m_propertiesPanel = nullptr;
    QString m_filePath;
    QString m_lastExportedSt;
    QString m_tempCompilePath;
    QString m_tempXmlCompilePath;
    bool m_useNativeXmlCompile = false;
    bool m_syncingSelection = false;
};
