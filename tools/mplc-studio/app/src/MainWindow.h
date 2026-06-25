/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "CompilerService.h"
#include "DeviceClient.h"
#include "DeviceDiscoveryService.h"
#include "DiscoveryDialog.h"
#include "DocumentEditor.h"
#include "RibbonUi.h"
#include "SerialMonitorWidget.h"
#include "StImportService.h"
#include "StInstructionToolbox.h"
#include "StTextEditor.h"
#include "StudioTheme.h"
#include "ladder/LadderEditorWidget.h"
#include "ladder/LadderPropertiesPanel.h"
#include "ladder/LadderScene.h"
#include "ladder/LadderToolbox.h"

#include <QMainWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QToolButton>

class QLineEdit;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void createActions();
    void createUi();
    void createRibbon();
    void loadSettings();
    void saveSettings();
    void updateWindowTitle();
    void appendLog(const QString &message);
    QString packagePathForSource(const QString &sourcePath) const;
    bool maybeSave();
    bool saveCurrentFile();
    void setCurrentFile(const QString &path);
    void openFilePath(const QString &path);
    void newStructuredTextDocument();
    void newLadderDocument();
    void activateEditorForPath(const QString &path);
    void updateSidePanelForEditor();
    void applyLadderSidePanelLayout(bool toolboxExpanded);
    bool compileCurrent();
    void handleCompileFinished(const CompileResult &result);
    void importStToLadder();
    void handleStImportFinished(const StLadderImporter::Result &result);
    void setBackgroundTaskUiEnabled(bool enabled);
    bool waitForBackgroundTasks();
    QString currentPackagePath() const;
    void discoverDevices();
    void refreshRibbonIcons();
    void showHelpDialog();
    void clearOutputLog();
    void showInfoDialog(const QString &title, const QString &message);
    void styleMessageBox(QMessageBox &box) const;
    void onEditorDirtyChanged(bool dirty);
    void cleanupTempCompileFile();

    CompilerService m_compiler;
    StImportService m_stImport;
    DeviceClient m_device;
    DeviceDiscoveryService m_discovery;

    StTextEditor *m_stEditor = nullptr;
    LadderEditorWidget *m_ladderEditor = nullptr;
    DocumentEditor *m_activeEditor = nullptr;
    QStackedWidget *m_editorStack = nullptr;
    QStackedWidget *m_sidePanelStack = nullptr;

    QPlainTextEdit *m_log = nullptr;
    SerialMonitorWidget *m_serialMonitor = nullptr;
    QTabWidget *m_bottomTabs = nullptr;
    StInstructionToolbox *m_stToolbox = nullptr;
    LadderToolbox *m_ladderToolbox = nullptr;
    QWidget *m_ladderSidePanel = nullptr;
    LadderPropertiesPanel *m_ladderPropertiesPanel = nullptr;
    QSplitter *m_workSplitter = nullptr;
    QLineEdit *m_deviceHostEdit = nullptr;
    QLineEdit *m_userEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLabel *m_statusLabel = nullptr;

    QToolButton *m_newButton = nullptr;
    QToolButton *m_newLadderButton = nullptr;
    QToolButton *m_openButton = nullptr;
    QToolButton *m_saveButton = nullptr;
    QToolButton *m_compileButton = nullptr;
    QToolButton *m_discoverButton = nullptr;
    QToolButton *m_uploadButton = nullptr;
    QToolButton *m_rebootButton = nullptr;
    QToolButton *m_addRungButton = nullptr;
    QToolButton *m_addContactButton = nullptr;
    QToolButton *m_importStToLadderButton = nullptr;

    QString m_currentFile;
    QString m_lastPackagePath;
    QString m_tempCompileCleanupPath;
    QString m_pendingStImportSuggestedPath;
    bool m_dirty = false;

    QAction *m_newAction = nullptr;
    QAction *m_newLadderAction = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_saveAsAction = nullptr;
    QAction *m_compileAction = nullptr;
    QAction *m_uploadAction = nullptr;
    QAction *m_rebootAction = nullptr;
    QAction *m_discoverAction = nullptr;
    QAction *m_clearLogAction = nullptr;
    QAction *m_addRungAction = nullptr;
    QAction *m_addContactAction = nullptr;
    QAction *m_viewGeneratedStAction = nullptr;
    QAction *m_importStToLadderAction = nullptr;
    QAction *m_quitAction = nullptr;
};
