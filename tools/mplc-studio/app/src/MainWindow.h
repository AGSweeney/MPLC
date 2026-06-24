/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "CompilerService.h"
#include "DeviceClient.h"
#include "DeviceDiscoveryService.h"
#include "DiscoveryDialog.h"
#include "RibbonUi.h"
#include "SerialMonitorWidget.h"
#include "StInstructionToolbox.h"
#include "StudioTheme.h"

#include <QMainWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSplitter>
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
    bool compileCurrent();
    QString currentPackagePath() const;
    void discoverDevices();
    void refreshRibbonIcons();
    void showHelpDialog();
    void clearOutputLog();
    void showInfoDialog(const QString &title, const QString &message);
    void styleMessageBox(QMessageBox &box) const;

    CompilerService m_compiler;
    DeviceClient m_device;
    DeviceDiscoveryService m_discovery;

    QPlainTextEdit *m_editor = nullptr;
    QPlainTextEdit *m_log = nullptr;
    SerialMonitorWidget *m_serialMonitor = nullptr;
    QTabWidget *m_bottomTabs = nullptr;
    StInstructionToolbox *m_toolbox = nullptr;
    QSplitter *m_workSplitter = nullptr;
    QLineEdit *m_deviceHostEdit = nullptr;
    QLineEdit *m_userEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLabel *m_statusLabel = nullptr;

    QToolButton *m_newButton = nullptr;
    QToolButton *m_openButton = nullptr;
    QToolButton *m_saveButton = nullptr;
    QToolButton *m_compileButton = nullptr;
    QToolButton *m_discoverButton = nullptr;
    QToolButton *m_uploadButton = nullptr;
    QToolButton *m_rebootButton = nullptr;

    QString m_currentFile;
    QString m_lastPackagePath;
    bool m_dirty = false;

    QAction *m_newAction = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_saveAsAction = nullptr;
    QAction *m_compileAction = nullptr;
    QAction *m_uploadAction = nullptr;
    QAction *m_rebootAction = nullptr;
    QAction *m_discoverAction = nullptr;
    QAction *m_clearLogAction = nullptr;
    QAction *m_quitAction = nullptr;
};
