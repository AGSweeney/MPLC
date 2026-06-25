/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderEditorWidget.h"
#include "LadderCanvasView.h"
#include "LadderElementEditorDialog.h"
#include "LadderInstructionBar.h"
#include "LadderPropertiesPanel.h"
#include "LadderScene.h"
#include "LadderStExporter.h"
#include "LadderPlcopenSerializer.h"
#include "StLadderImporter.h"
#include "LadderValidator.h"
#include "LadderVariableBrowserDialog.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QGraphicsItem>
#include <QMessageBox>
#include <QScrollBar>
#include <QVBoxLayout>

LadderEditorWidget::LadderEditorWidget(QWidget *parent)
    : DocumentEditor(Kind::Ladder, parent)
    , m_useNativeXmlCompile(true)
{
    setObjectName("LadderEditor");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_instructionBar = new LadderInstructionBar(this);
    layout->addWidget(m_instructionBar);

    m_scene = new LadderScene(this);
    m_view = new LadderCanvasView(m_scene, this);
    m_view->setMinimumHeight(120);
    layout->addWidget(m_view, 1);
    m_view->updateSceneLayoutWidth();

    m_propertiesPanel = new LadderPropertiesPanel(this);

    connect(m_instructionBar, &LadderInstructionBar::addContact, this, [this](bool negated) {
        m_scene->addContactToSelectedRung(negated);
    });
    connect(m_instructionBar, &LadderInstructionBar::addCoil, this, [this]() {
        m_scene->addCoilToSelectedRung(LadderCoilKind::Normal);
    });
    connect(m_instructionBar, &LadderInstructionBar::addSetCoil, this, [this]() {
        m_scene->addCoilToSelectedRung(LadderCoilKind::Set);
    });
    connect(m_instructionBar, &LadderInstructionBar::addResetCoil, this, [this]() {
        m_scene->addCoilToSelectedRung(LadderCoilKind::Reset);
    });
    connect(m_instructionBar, &LadderInstructionBar::addFunctionBlock, this, [this](const QString &typeName) {
        m_scene->addFunctionBlockToSelectedRung(typeName);
    });
    connect(m_instructionBar, &LadderInstructionBar::addRung, this, [this]() {
        m_scene->addRung();
    });
    connect(m_instructionBar, &LadderInstructionBar::surroundWithBranch, this, [this]() {
        m_scene->surroundSelectionWithBranch();
    });
    connect(m_instructionBar, &LadderInstructionBar::addParallelBranchLeg, this, [this]() {
        m_scene->addParallelBranchLeg(false);
    });
    connect(m_instructionBar, &LadderInstructionBar::removeBranch, this, [this]() {
        m_scene->removeBranchFromSelectedRung();
    });
    connect(m_instructionBar, &LadderInstructionBar::deleteSelection, this, [this]() {
        m_scene->removeSelectedElements();
    });
    connect(m_instructionBar, &LadderInstructionBar::deleteRung, this, [this]() {
        m_scene->removeSelectedRung();
    });
    connect(m_instructionBar, &LadderInstructionBar::viewGeneratedSt, this, &LadderEditorWidget::showGeneratedSt);
    connect(m_instructionBar, &LadderInstructionBar::browseVariables, this, &LadderEditorWidget::showVariableBrowser);

    connect(m_view, &LadderCanvasView::addContactRequested, this, [this](bool negated) {
        m_scene->addContactToSelectedRung(negated);
    });
    connect(m_view, &LadderCanvasView::addCoilRequested, this, [this]() {
        m_scene->addCoilToSelectedRung(LadderCoilKind::Normal);
    });
    connect(m_view, &LadderCanvasView::addRungRequested, this, [this]() {
        m_scene->addRung();
    });
    connect(m_view, &LadderCanvasView::surroundWithBranchRequested, this, [this]() {
        m_scene->surroundSelectionWithBranch();
    });
    connect(m_view, &LadderCanvasView::removeBranchRequested, this, [this]() {
        m_scene->removeBranchFromSelectedRung();
    });
    connect(m_view, &LadderCanvasView::addParallelBranchLegRequested, this, [this](bool negated) {
        m_scene->addParallelBranchLeg(negated);
    });
    connect(m_view, &LadderCanvasView::addContactAtCellRequested, this,
            [this](int rungIndex, int row, int col, bool negated) {
                m_scene->addContactAtCell(rungIndex, row, col, negated);
            });
    connect(m_view, &LadderCanvasView::deleteRequested, this, [this]() {
        m_scene->handleDeleteKey();
    });

    connect(m_scene, &LadderScene::programModified, this, &LadderEditorWidget::onProgramModified);
    connect(m_scene, &LadderScene::statusMessage, this, &LadderEditorWidget::statusMessage);
    connect(m_scene, &LadderScene::elementActivated, this, [this]() { updateSelection(); });
    connect(m_scene, &LadderScene::elementDoubleClicked, this, &LadderEditorWidget::handleDoubleClick);
    connect(m_scene, &QGraphicsScene::selectionChanged, this, &LadderEditorWidget::updateSelection);
    connect(m_propertiesPanel, &LadderPropertiesPanel::propertiesChanged, this, [this]() {
        onProgramModified();
    });
    connect(m_propertiesPanel, &LadderPropertiesPanel::editRequested, this, [this]() {
        const int localId = m_propertiesPanel->selectedLocalId();
        if (LadderElementItem *item = m_scene->elementItemByLocalId(localId)) {
            handleDoubleClick(item);
        }
    });

    newDocument();
}

void LadderEditorWidget::rebuildUi()
{
    m_propertiesPanel->setProgram(&m_model.program);
    m_scene->setProgram(&m_model.program);
    if (m_view != nullptr) {
        m_view->resetZoom();
        m_view->verticalScrollBar()->setValue(0);
        m_view->horizontalScrollBar()->setValue(0);
    }
    onProgramModified();
}

void LadderEditorWidget::updateSelection()
{
    if (m_syncingSelection) {
        return;
    }

    LadderElementItem *selected = nullptr;
    for (QGraphicsItem *item : m_scene->selectedItems()) {
        if (auto *elementItem = dynamic_cast<LadderElementItem *>(item)) {
            selected = elementItem;
            break;
        }
    }
    m_propertiesPanel->setSelectedItem(selected);
    emit selectionChanged(selected);
}

void LadderEditorWidget::refreshElementPresentation(int localId, bool reselect)
{
    m_syncingSelection = true;
    m_scene->refreshElementItem(localId);

    LadderElementItem *item = m_scene->elementItemByLocalId(localId);
    if (reselect && item != nullptr) {
        m_scene->clearSelection();
        item->setSelected(true);
    }

    m_propertiesPanel->setSelectedItem(item);
    emit selectionChanged(item);
    m_syncingSelection = false;
}

void LadderEditorWidget::handleDoubleClick(LadderElementItem *item)
{
    if (item == nullptr) {
        return;
    }

    const int localId = item->localId();
    LadderElement *element = m_model.program.elementById(localId);
    if (element == nullptr) {
        return;
    }

    if (!LadderElementEditorDialog::editElement(&m_model.program, element, this)) {
        return;
    }

    refreshElementPresentation(localId);
    onProgramModified();
}

void LadderEditorWidget::onProgramModified()
{
    setDirty(true);

    const QVector<LadderValidator::Issue> issues = LadderValidator::validate(m_model.program);
    for (const LadderValidator::Issue &issue : issues) {
        if (issue.severity == LadderValidator::Issue::Severity::Error) {
            emit statusMessage(QStringLiteral("Ladder: %1").arg(issue.message));
            break;
        }
    }

    const int localId = m_propertiesPanel->selectedLocalId();
    if (localId > 0) {
        m_scene->refreshElementItem(localId);
        if (LadderElementItem *item = m_scene->elementItemByLocalId(localId)) {
            m_syncingSelection = true;
            m_propertiesPanel->setSelectedItem(item);
            m_syncingSelection = false;
        } else {
            m_syncingSelection = true;
            m_propertiesPanel->setSelectedItem(nullptr);
            m_syncingSelection = false;
        }
    }
}

bool LadderEditorWidget::loadFromPath(const QString &path)
{
    QString errorMessage;
    if (!m_model.loadFromPath(path, &errorMessage)) {
        emit statusMessage(errorMessage);
        return false;
    }
    m_filePath = QFileInfo(path).absoluteFilePath();
    rebuildUi();
    setDirty(false);
    return true;
}

bool LadderEditorWidget::saveToPath(const QString &path)
{
    QString errorMessage;
    if (!m_model.saveToPath(path, &errorMessage)) {
        emit statusMessage(errorMessage);
        return false;
    }
    m_filePath = QFileInfo(path).absoluteFilePath();
    setDirty(false);
    return true;
}

void LadderEditorWidget::markClean()
{
    setDirty(false);
}

void LadderEditorWidget::newDocument()
{
    m_model.program.newDefaultProgram();
    m_filePath.clear();
    m_lastExportedSt.clear();
    rebuildUi();
    setDirty(true);
}

QString LadderEditorWidget::suggestedSaveFilter() const
{
    return QStringLiteral("Ladder Project (*.xml)");
}

QString LadderEditorWidget::suggestedOpenFilter() const
{
    return QStringLiteral("Ladder Project (*.xml)");
}

bool LadderEditorWidget::exportCompileSource(QString *errorMessage)
{
    const LadderStExporter::Result exported = LadderStExporter::exportProgram(m_model.program);
    if (!exported.ok) {
        if (errorMessage != nullptr) {
            *errorMessage = exported.diagnostics.join('\n');
        }
        return false;
    }

    m_lastExportedSt = exported.source;
    const QString basePath = m_filePath.isEmpty() ? QDir::tempPath() + "/ladder_export" : m_filePath;
    m_tempCompilePath = QFileInfo(QFileInfo(basePath).absolutePath(),
                                  QFileInfo(basePath).completeBaseName() + "_ladder_export.st")
                            .absoluteFilePath();

    QFile file(m_tempCompilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    if (file.write(m_lastExportedSt.toUtf8()) < 0) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    return true;
}

bool LadderEditorWidget::exportCompileXml(QString *errorMessage)
{
    const QString basePath = m_filePath.isEmpty() ? QDir::tempPath() + "/ladder_export" : m_filePath;
    m_tempXmlCompilePath = QFileInfo(QFileInfo(basePath).absolutePath(),
                                     QFileInfo(basePath).completeBaseName() + "_ladder_export.xml")
                               .absoluteFilePath();

    QString saveError;
    if (!LadderPlcopenSerializer::save(m_tempXmlCompilePath, m_model.program, &saveError)) {
        if (errorMessage != nullptr) {
            *errorMessage = saveError;
        }
        return false;
    }
    return true;
}

QString LadderEditorWidget::compileInputPath(QString *tempCleanupPath)
{
    if (tempCleanupPath != nullptr) {
        tempCleanupPath->clear();
    }

    if (m_useNativeXmlCompile) {
        QString errorMessage;
        if (!m_filePath.isEmpty()) {
            return m_filePath;
        }
        if (!exportCompileXml(&errorMessage)) {
            emit statusMessage(errorMessage);
            return QString();
        }
        if (tempCleanupPath != nullptr) {
            *tempCleanupPath = m_tempXmlCompilePath;
        }
        return m_tempXmlCompilePath;
    }

    QString errorMessage;
    if (!exportCompileSource(&errorMessage)) {
        emit statusMessage(errorMessage);
        return QString();
    }

    if (tempCleanupPath != nullptr) {
        *tempCleanupPath = m_tempCompilePath;
    }
    return m_tempCompilePath;
}

void LadderEditorWidget::showGeneratedSt()
{
    QString errorMessage;
    if (!exportCompileSource(&errorMessage)) {
        QMessageBox::warning(this, QStringLiteral("Export Failed"), errorMessage);
        return;
    }

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("Generated ST"));
    box.setText(QStringLiteral("Structured Text generated from the ladder program:"));
    box.setDetailedText(m_lastExportedSt);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

void LadderEditorWidget::showVariableBrowser()
{
    LadderVariableBrowserDialog dialog(&m_model.program, this);
    connect(&dialog, &LadderVariableBrowserDialog::programModified, this, [this]() {
        onProgramModified();
        m_propertiesPanel->setProgram(&m_model.program);
    });
    dialog.exec();
    m_scene->refreshAllElementItems();
    m_propertiesPanel->setProgram(&m_model.program);
    updateSelection();
}

void LadderEditorWidget::setUseNativeXmlCompile(bool enabled)
{
    m_useNativeXmlCompile = enabled;
}

bool LadderEditorWidget::importFromStSource(const QString &source, QStringList *diagnostics)
{
    const StLadderImporter::Result imported = StLadderImporter::importSource(source);
    if (diagnostics != nullptr) {
        *diagnostics = imported.diagnostics;
    }

    if (!imported.ok) {
        const QString message = imported.diagnostics.isEmpty()
                                    ? QStringLiteral("ST import failed.")
                                    : imported.diagnostics.join('\n');
        emit statusMessage(message);
        return false;
    }

    applyImportedProgram(imported.program);

    if (!imported.diagnostics.isEmpty()) {
        emit statusMessage(imported.diagnostics.join('\n'));
    }
    return true;
}

void LadderEditorWidget::applyImportedProgram(const LadderProgram &program)
{
    m_model.program = program;
    m_filePath.clear();
    rebuildUi();
    setDirty(true);
}

bool LadderEditorWidget::importFromStPath(const QString &path, QStringList *diagnostics)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString message = file.errorString();
        if (diagnostics != nullptr) {
            diagnostics->push_back(message);
        }
        emit statusMessage(message);
        return false;
    }

    return importFromStSource(QString::fromUtf8(file.readAll()), diagnostics);
}
