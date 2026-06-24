/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "MainWindow.h"
#include "SerialMonitorWidget.h"
#include "StInstructionToolbox.h"
#include "StSyntaxHighlighter.h"

#include <QAction>
#include <QCloseEvent>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("MPLC Studio");
    setWindowIcon(QIcon(":/icons/app-icon-256.png"));
    resize(1200, 820);

    createActions();
    createUi();
    createRibbon();
    loadSettings();
    updateWindowTitle();

    connect(&m_device, &DeviceClient::logMessage, this, &MainWindow::appendLog);
    connect(&m_device, &DeviceClient::operationFailed, this, [this](const QString &detail) {
        QMessageBox box(QMessageBox::Warning, "Device Operation Failed", detail, QMessageBox::Ok, this);
        styleMessageBox(box);
        box.exec();
    });
    connect(&m_device, &DeviceClient::uploadFinished, this, [this](bool success, const QString &detail) {
        if (!success) {
            return;
        }
        showInfoDialog("Upload Complete",
                       QString("Package uploaded to %1.\n\nReboot the target to run the new program.")
                           .arg(m_deviceHostEdit->text().trimmed()));
        Q_UNUSED(detail);
    });
    connect(&m_device, &DeviceClient::rebootFinished, this, [this](bool success, const QString &detail) {
        if (!success) {
            return;
        }
        showInfoDialog("Reboot Requested",
                       QString("Reboot command sent to %1.\n\nWatch the Serial Monitor for boot messages.")
                           .arg(m_deviceHostEdit->text().trimmed()));
        Q_UNUSED(detail);
    });
    connect(&m_discovery, &DeviceDiscoveryService::logMessage, this, &MainWindow::appendLog);
    connect(m_serialMonitor, &SerialMonitorWidget::statusMessage, this, &MainWindow::appendLog);
}

void MainWindow::createActions()
{
    m_newAction = new QAction("New", this);
    m_newAction->setShortcut(QKeySequence::New);
    connect(m_newAction, &QAction::triggered, this, [this]() {
        if (!maybeSave()) {
            return;
        }
        m_editor->clear();
        m_currentFile.clear();
        m_lastPackagePath.clear();
        m_dirty = false;
        updateWindowTitle();
        appendLog("New ST document.");
    });

    m_openAction = new QAction("Open...", this);
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, [this]() {
        if (!maybeSave()) {
            return;
        }
        const QString path = QFileDialog::getOpenFileName(
            this,
            "Open Structured Text",
            m_currentFile.isEmpty() ? QString() : QFileInfo(m_currentFile).absolutePath(),
            "Structured Text (*.st);;All Files (*)");
        if (!path.isEmpty()) {
            openFilePath(path);
        }
    });

    m_saveAction = new QAction("Save", this);
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, [this]() {
        saveCurrentFile();
    });

    m_saveAsAction = new QAction("Save As...", this);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(m_saveAsAction, &QAction::triggered, this, [this]() {
        const QString path = QFileDialog::getSaveFileName(
            this,
            "Save Structured Text",
            m_currentFile.isEmpty() ? "program.st" : m_currentFile,
            "Structured Text (*.st)");
        if (!path.isEmpty()) {
            setCurrentFile(path);
            saveCurrentFile();
        }
    });

    m_compileAction = new QAction("Compile", this);
    m_compileAction->setShortcut(Qt::CTRL | Qt::Key_B);
    connect(m_compileAction, &QAction::triggered, this, [this]() {
        compileCurrent();
    });

    m_uploadAction = new QAction("Upload", this);
    connect(m_uploadAction, &QAction::triggered, this, [this]() {
        saveSettings();
        const QString packagePath = currentPackagePath();
        if (packagePath.isEmpty()) {
            QMessageBox box(QMessageBox::Information,
                            "Upload",
                            "Compile the program first or choose a .mplc file.",
                            QMessageBox::Ok,
                            this);
            styleMessageBox(box);
            box.exec();
            return;
        }
        m_device.uploadPackage(packagePath);
    });

    m_rebootAction = new QAction("Reboot Target", this);
    connect(m_rebootAction, &QAction::triggered, this, [this]() {
        saveSettings();
        m_device.rebootDevice();
    });

    m_discoverAction = new QAction("Discover Devices...", this);
    connect(m_discoverAction, &QAction::triggered, this, &MainWindow::discoverDevices);

    m_clearLogAction = new QAction("Clear Output", this);
    m_clearLogAction->setShortcut(Qt::CTRL | Qt::Key_L);
    connect(m_clearLogAction, &QAction::triggered, this, &MainWindow::clearOutputLog);

    m_quitAction = new QAction("Quit", this);
    m_quitAction->setShortcut(QKeySequence::Quit);
    connect(m_quitAction, &QAction::triggered, this, [this]() {
        close();
    });
}

void MainWindow::createUi()
{
    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(3);

    m_deviceHostEdit = new QLineEdit(central);
    m_deviceHostEdit->setPlaceholderText("Target IP (e.g. 192.168.1.210)");
    m_deviceHostEdit->setMinimumWidth(160);
    m_deviceHostEdit->setMaximumWidth(220);

    m_userEdit = new QLineEdit(central);
    m_userEdit->setPlaceholderText("HTTP user (optional)");
    m_userEdit->setMaximumWidth(140);

    m_passwordEdit = new QLineEdit(central);
    m_passwordEdit->setPlaceholderText("HTTP password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setMaximumWidth(140);

    m_editor = new QPlainTextEdit(central);
    m_editor->setObjectName("CodeEditor");
    m_editor->setPlaceholderText("Write Structured Text here...");
    m_editor->setTabStopDistance(48);
    m_editor->setFont(QFont(QStringLiteral("Consolas"), 13));
    new StSyntaxHighlighter(m_editor->document());

    m_log = new QPlainTextEdit(central);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(500);

    auto *outputPane = new QWidget(central);
    auto *outputLayout = new QVBoxLayout(outputPane);
    outputLayout->setContentsMargins(6, 6, 6, 6);
    outputLayout->setSpacing(6);

    auto *outputToolbar = new QHBoxLayout();
    outputToolbar->setSpacing(6);
    auto *outputClearButton = new QPushButton("Clear", outputPane);
    outputClearButton->setObjectName("ToolbarButton");
    outputClearButton->setFixedHeight(28);
    outputToolbar->addWidget(outputClearButton);
    outputToolbar->addStretch(1);
    connect(outputClearButton, &QPushButton::clicked, this, &MainWindow::clearOutputLog);

    outputLayout->addLayout(outputToolbar);
    outputLayout->addWidget(m_log, 1);

    m_serialMonitor = new SerialMonitorWidget(central);

    m_bottomTabs = new QTabWidget(central);
    m_bottomTabs->setObjectName("BottomTabs");
    m_bottomTabs->addTab(outputPane, "Output");
    m_bottomTabs->addTab(m_serialMonitor, "Serial Monitor");

    auto *editorPane = new QWidget(central);
    auto *editorPaneLayout = new QVBoxLayout(editorPane);
    editorPaneLayout->setContentsMargins(0, 0, 0, 0);
    editorPaneLayout->setSpacing(3);

    auto *endpointRow = new QHBoxLayout();
    endpointRow->setContentsMargins(8, 4, 8, 0);
    endpointRow->setSpacing(8);
    auto *endpointLabel = new QLabel("Target / IP", editorPane);
    endpointLabel->setObjectName("EndpointLabel");
    endpointRow->addWidget(endpointLabel);
    endpointRow->addWidget(m_deviceHostEdit);
    auto *userLabel = new QLabel("User", editorPane);
    userLabel->setObjectName("EndpointLabel");
    endpointRow->addWidget(userLabel);
    endpointRow->addWidget(m_userEdit);
    auto *passLabel = new QLabel("Pass", editorPane);
    passLabel->setObjectName("EndpointLabel");
    endpointRow->addWidget(passLabel);
    endpointRow->addWidget(m_passwordEdit);
    endpointRow->addStretch(1);

    editorPaneLayout->addLayout(endpointRow);
    editorPaneLayout->addWidget(m_editor, 1);

    auto *logToggleBar = new QWidget(central);
    logToggleBar->setObjectName("LogToggleBar");
    logToggleBar->setFixedHeight(26);
    auto *logToggleLayout = new QHBoxLayout(logToggleBar);
    logToggleLayout->setContentsMargins(10, 0, 6, 0);
    logToggleLayout->setSpacing(6);

    auto *logToggleLabel = new QLabel("Output / Serial", logToggleBar);
    logToggleLabel->setObjectName("LogToggleLabel");

    auto *logClearBtn = new QPushButton("Clear", logToggleBar);
    logClearBtn->setObjectName("LogToggleButton");
    logClearBtn->setFixedHeight(20);
    logClearBtn->setFlat(true);
    logClearBtn->setCursor(Qt::PointingHandCursor);
    connect(logClearBtn, &QPushButton::clicked, this, &MainWindow::clearOutputLog);

    auto *logToggleBtn = new QPushButton("▼  Hide", logToggleBar);
    logToggleBtn->setObjectName("LogToggleButton");
    logToggleBtn->setFixedHeight(20);
    logToggleBtn->setFlat(true);
    logToggleBtn->setCursor(Qt::PointingHandCursor);

    logToggleLayout->addWidget(logToggleLabel);
    logToggleLayout->addWidget(logClearBtn);
    logToggleLayout->addStretch(1);
    logToggleLayout->addWidget(logToggleBtn);

    auto *contentSplitter = new QSplitter(Qt::Vertical, central);
    contentSplitter->addWidget(editorPane);
    contentSplitter->addWidget(m_bottomTabs);
    contentSplitter->setStretchFactor(0, 5);
    contentSplitter->setStretchFactor(1, 1);
    contentSplitter->setCollapsible(1, true);
    contentSplitter->setSizes({640, 140});

    auto *savedLogSize = new int(140);
    connect(logToggleBtn, &QPushButton::clicked, this, [contentSplitter, logToggleBtn, savedLogSize]() {
        const QList<int> sizes = contentSplitter->sizes();
        const bool logVisible = sizes[1] > 0;
        if (logVisible) {
            *savedLogSize = sizes[1];
            contentSplitter->setSizes({sizes[0] + sizes[1], 0});
            logToggleBtn->setText("▶  Show");
        } else {
            const int restore = (*savedLogSize > 40) ? *savedLogSize : 140;
            const int total = sizes[0] + sizes[1];
            contentSplitter->setSizes({total - restore, restore});
            logToggleBtn->setText("▼  Hide");
        }
    });

    auto *leftColumn = new QWidget(central);
    auto *leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    leftLayout->addWidget(contentSplitter, 1);
    leftLayout->addWidget(logToggleBar);

    m_toolbox = new StInstructionToolbox(m_editor, central);
    m_workSplitter = new QSplitter(Qt::Horizontal, central);
    m_workSplitter->addWidget(leftColumn);
    m_workSplitter->addWidget(m_toolbox);
    m_workSplitter->setStretchFactor(0, 1);
    m_workSplitter->setStretchFactor(1, 0);
    m_workSplitter->setCollapsible(1, true);
    m_workSplitter->setSizes({860, 240});

    auto *savedToolboxWidth = new int(240);
    connect(m_toolbox, &StInstructionToolbox::expandedChanged, this, [this, savedToolboxWidth](bool expanded) {
        const QList<int> sizes = m_workSplitter->sizes();
        if (expanded) {
            const int restore = (*savedToolboxWidth > 80) ? *savedToolboxWidth : 240;
            const int total = sizes[0] + sizes[1];
            m_workSplitter->setSizes({total - restore, restore});
        } else {
            *savedToolboxWidth = sizes[1];
            m_workSplitter->setSizes({sizes[0] + sizes[1], 24});
        }
    });

    rootLayout->addWidget(m_workSplitter, 1);

    setCentralWidget(central);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("StatusLabel");
    m_statusLabel->setAutoFillBackground(false);
    statusBar()->addPermanentWidget(m_statusLabel);

    connect(m_editor, &QPlainTextEdit::textChanged, this, [this]() {
        if (!m_dirty) {
            m_dirty = true;
            updateWindowTitle();
        }
    });
}

void MainWindow::createRibbon()
{
    auto *central = centralWidget();
    auto *rootLayout = static_cast<QVBoxLayout *>(central->layout());

    const QColor ribbonIconColor("#1a1a1a");

    m_newButton = makeRibbonCommandButton("New", ":/icons/star-plus.svg", this, ribbonIconColor);
    m_openButton = makeRibbonCommandButton("Open", ":/icons/list.svg", this, ribbonIconColor);
    m_saveButton = makeRibbonCommandButton("Save", ":/icons/chevron-up.svg", this, ribbonIconColor);
    m_compileButton = makeRibbonCommandButton("Compile", ":/icons/list.svg", this, ribbonIconColor);
    m_discoverButton = makeRibbonCommandButton("Discover", ":/icons/search.svg", this, ribbonIconColor);
    m_uploadButton = makeRibbonCommandButton("Upload", ":/icons/link.svg", this, ribbonIconColor);
    m_rebootButton = makeRibbonCommandButton("Reboot", ":/icons/unlink.svg", this, ribbonIconColor);

    auto *helpButton = makeRibbonCommandButton("About", ":/icons/app-icon-64.png", this, ribbonIconColor);
    helpButton->setToolTip("About MPLC Studio");

    connect(m_newButton, &QToolButton::clicked, m_newAction, &QAction::trigger);
    connect(m_openButton, &QToolButton::clicked, m_openAction, &QAction::trigger);
    connect(m_saveButton, &QToolButton::clicked, m_saveAction, &QAction::trigger);
    connect(m_compileButton, &QToolButton::clicked, m_compileAction, &QAction::trigger);
    connect(m_discoverButton, &QToolButton::clicked, m_discoverAction, &QAction::trigger);
    connect(m_uploadButton, &QToolButton::clicked, m_uploadAction, &QAction::trigger);
    connect(m_rebootButton, &QToolButton::clicked, m_rebootAction, &QAction::trigger);
    connect(helpButton, &QToolButton::clicked, this, &MainWindow::showHelpDialog);

    auto *acadRibbon = new QWidget(this);
    acadRibbon->setObjectName("AcadRibbon");
    acadRibbon->setFixedHeight(130);
    auto *acadLayout = new QVBoxLayout(acadRibbon);
    acadLayout->setContentsMargins(0, 0, 0, 0);
    acadLayout->setSpacing(0);

    auto *tabRow = new QWidget(acadRibbon);
    tabRow->setObjectName("RibbonTabRow");
    tabRow->setFixedHeight(28);
    auto *tabRowLayout = new QHBoxLayout(tabRow);
    tabRowLayout->setContentsMargins(0, 0, 0, 0);
    tabRowLayout->setSpacing(0);

    auto *appMenuButton = new QToolButton(tabRow);
    appMenuButton->setObjectName("ApplicationMenuButton");
    appMenuButton->setIcon(QIcon(":/icons/app-icon-64.png"));
    appMenuButton->setPopupMode(QToolButton::InstantPopup);
    appMenuButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    appMenuButton->setIconSize(QSize(18, 18));
    appMenuButton->setFixedSize(38, 28);

    auto *appMenu = new QMenu(appMenuButton);
    appMenu->addAction(m_newAction);
    appMenu->addAction(m_openAction);
    appMenu->addAction(m_saveAction);
    appMenu->addAction(m_saveAsAction);
    appMenu->addSeparator();
    appMenu->addAction(m_compileAction);
    appMenu->addAction(m_discoverAction);
    appMenu->addAction(m_uploadAction);
    appMenu->addAction(m_rebootAction);
    appMenu->addSeparator();
    appMenu->addAction(m_clearLogAction);
    appMenu->addSeparator();
    appMenu->addAction("About", this, &MainWindow::showHelpDialog);
    appMenu->addSeparator();
    appMenu->addAction(m_quitAction);
    appMenuButton->setMenu(appMenu);

    auto *homeTabButton = makeRibbonTabButton("Home", tabRow);
    auto *helpTabButton = makeRibbonTabButton("Help", tabRow);
    homeTabButton->setChecked(true);

    tabRowLayout->addWidget(appMenuButton);
    tabRowLayout->addWidget(homeTabButton);
    tabRowLayout->addWidget(helpTabButton);
    tabRowLayout->addStretch(1);

    auto *ribbonPages = new QStackedWidget(acadRibbon);
    ribbonPages->setObjectName("RibbonPages");

    auto *homePage = new QWidget();
    homePage->setObjectName("RibbonBar");
    auto *homeLayout = new QHBoxLayout(homePage);
    homeLayout->setContentsMargins(4, 2, 4, 2);
    homeLayout->setSpacing(0);

    const RibbonGroup fileGroup = makeRibbonGroup(homePage, "File");
    fileGroup.commands->addWidget(m_newButton);
    fileGroup.commands->addWidget(m_openButton);
    fileGroup.commands->addWidget(m_saveButton);

    const RibbonGroup findGroup = makeRibbonGroup(homePage, "Find");
    findGroup.commands->addWidget(m_discoverButton);

    const RibbonGroup deviceGroup = makeRibbonGroup(homePage, "Device");
    deviceGroup.commands->addWidget(m_compileButton);
    deviceGroup.commands->addWidget(m_uploadButton);
    deviceGroup.commands->addWidget(m_rebootButton);

    homeLayout->addWidget(fileGroup.widget);
    homeLayout->addWidget(findGroup.widget);
    homeLayout->addWidget(deviceGroup.widget);
    homeLayout->addStretch(1);

    auto *helpPage = new QWidget();
    helpPage->setObjectName("RibbonBar");
    auto *helpLayout = new QHBoxLayout(helpPage);
    helpLayout->setContentsMargins(4, 2, 4, 2);
    helpLayout->setSpacing(0);

    const RibbonGroup infoGroup = makeRibbonGroup(helpPage, "Information");
    infoGroup.commands->addWidget(helpButton);
    helpLayout->addWidget(infoGroup.widget);
    helpLayout->addStretch(1);

    ribbonPages->addWidget(homePage);
    ribbonPages->addWidget(helpPage);

    acadLayout->addWidget(tabRow);
    acadLayout->addWidget(ribbonPages, 1);

    const auto switchRibbonTab = [ribbonPages, homeTabButton, helpTabButton](int index, QPushButton *activeButton) {
        ribbonPages->setCurrentIndex(index);
        homeTabButton->setChecked(false);
        helpTabButton->setChecked(false);
        activeButton->setChecked(true);
    };
    connect(homeTabButton, &QPushButton::clicked, this, [switchRibbonTab, homeTabButton]() {
        switchRibbonTab(0, homeTabButton);
    });
    connect(helpTabButton, &QPushButton::clicked, this, [switchRibbonTab, helpTabButton]() {
        switchRibbonTab(1, helpTabButton);
    });

    rootLayout->insertWidget(0, acadRibbon);
}

void MainWindow::refreshRibbonIcons()
{
    const QColor iconColor("#1a1a1a");
    const int edge = 36;

    if (m_newButton != nullptr) {
        m_newButton->setIcon(StudioTheme::renderSvgIcon(":/icons/star-plus.svg", edge, iconColor));
    }
    if (m_openButton != nullptr) {
        m_openButton->setIcon(StudioTheme::renderSvgIcon(":/icons/list.svg", edge, iconColor));
    }
    if (m_saveButton != nullptr) {
        m_saveButton->setIcon(StudioTheme::renderSvgIcon(":/icons/chevron-up.svg", edge, iconColor));
    }
    if (m_compileButton != nullptr) {
        m_compileButton->setIcon(StudioTheme::renderSvgIcon(":/icons/list.svg", edge, iconColor));
    }
    if (m_discoverButton != nullptr) {
        m_discoverButton->setIcon(StudioTheme::renderSvgIcon(":/icons/search.svg", edge, iconColor));
    }
    if (m_uploadButton != nullptr) {
        m_uploadButton->setIcon(StudioTheme::renderSvgIcon(":/icons/link.svg", edge, iconColor));
    }
    if (m_rebootButton != nullptr) {
        m_rebootButton->setIcon(StudioTheme::renderSvgIcon(":/icons/unlink.svg", edge, iconColor));
    }
}

void MainWindow::showHelpDialog()
{
    QMessageBox box(this);
    box.setWindowTitle("About MPLC Studio");
    box.setIconPixmap(QPixmap(":/icons/app-icon-256.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    box.setText("<b>MPLC Studio</b>");
    box.setInformativeText(
        "Edit Structured Text, compile in-process to .mplc packages, deploy to NetBurner MOD54415 targets, "
        "and monitor the target UART from the Serial Monitor tab.");
    styleMessageBox(box);
    box.exec();
}

void MainWindow::styleMessageBox(QMessageBox &box) const
{
    StudioTheme::styleMessageBox(box);
}

QString MainWindow::packagePathForSource(const QString &sourcePath) const
{
    const QFileInfo info(sourcePath);
    return QFileInfo(info.absolutePath(), info.completeBaseName() + ".mplc").absoluteFilePath();
}

QString MainWindow::currentPackagePath() const
{
    if (!m_lastPackagePath.isEmpty() && QFileInfo::exists(m_lastPackagePath)) {
        return m_lastPackagePath;
    }
    if (!m_currentFile.isEmpty()) {
        const QString derived = packagePathForSource(m_currentFile);
        if (QFileInfo::exists(derived)) {
            return derived;
        }
    }
    return QString();
}

void MainWindow::loadSettings()
{
    QSettings settings;
    m_deviceHostEdit->setText(settings.value("device/host", "192.168.1.210").toString());
    m_userEdit->setText(settings.value("device/user").toString());
    m_passwordEdit->setText(settings.value("device/password").toString());

    StudioTheme::apply(this);
    StudioTheme::styleLogView(m_log);
    refreshRibbonIcons();

    if (m_serialMonitor != nullptr) {
        m_serialMonitor->loadSettings();
    }

    m_device.setDeviceHost(m_deviceHostEdit->text().trimmed());
    m_device.setCredentials(m_userEdit->text(), m_passwordEdit->text());

    if (m_toolbox != nullptr) {
        const bool toolboxExpanded = settings.value("ui/toolboxExpanded", true).toBool();
        if (!toolboxExpanded) {
            m_toolbox->setExpanded(false);
        }
    }

    const QString lastFile = settings.value("editor/lastFile").toString();
    if (!lastFile.isEmpty() && QFileInfo::exists(lastFile)) {
        openFilePath(lastFile);
    } else {
        m_editor->setPlainText(
            "PROGRAM Main\n"
            "VAR\n"
            "    Led1 AT %QX0.0 : BOOL;\n"
            "END_VAR\n"
            "    Led1 := TRUE;\n"
            "END_PROGRAM\n");
        m_dirty = true;
        updateWindowTitle();
    }
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("device/host", m_deviceHostEdit->text().trimmed());
    settings.setValue("device/user", m_userEdit->text());
    settings.setValue("device/password", m_passwordEdit->text());
    if (!m_currentFile.isEmpty()) {
        settings.setValue("editor/lastFile", m_currentFile);
    }
    if (m_toolbox != nullptr) {
        settings.setValue("ui/toolboxExpanded", m_toolbox->isExpanded());
    }
    if (m_serialMonitor != nullptr) {
        m_serialMonitor->saveSettings();
    }

    m_device.setDeviceHost(m_deviceHostEdit->text().trimmed());
    m_device.setCredentials(m_userEdit->text(), m_passwordEdit->text());
}

void MainWindow::updateWindowTitle()
{
    QString title = "MPLC Studio";
    if (!m_currentFile.isEmpty()) {
        title = QFileInfo(m_currentFile).fileName();
        if (m_dirty) {
            title += "*";
        }
        title += " - MPLC Studio";
    } else if (m_dirty) {
        title = "Untitled* - MPLC Studio";
    }
    setWindowTitle(title);

    QString status = m_currentFile.isEmpty() ? "No file" : m_currentFile;
    if (!m_lastPackagePath.isEmpty()) {
        status += " | package: " + m_lastPackagePath;
    }
    m_statusLabel->setText(status);
}

void MainWindow::appendLog(const QString &message)
{
    m_log->appendPlainText(message);
}

void MainWindow::clearOutputLog()
{
    if (m_log != nullptr) {
        m_log->clear();
    }
}

void MainWindow::showInfoDialog(const QString &title, const QString &message)
{
    QMessageBox box(QMessageBox::Information, title, message, QMessageBox::Ok, this);
    styleMessageBox(box);
    box.exec();
}

bool MainWindow::maybeSave()
{
    if (!m_dirty) {
        return true;
    }

    QMessageBox box(QMessageBox::Question,
                    "Save Changes",
                    "Save changes before continuing?",
                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                    this);
    styleMessageBox(box);
    const QMessageBox::StandardButton choice = static_cast<QMessageBox::StandardButton>(box.exec());
    if (choice == QMessageBox::Save) {
        return saveCurrentFile();
    }
    if (choice == QMessageBox::Discard) {
        return true;
    }
    return false;
}

bool MainWindow::saveCurrentFile()
{
    if (m_currentFile.isEmpty()) {
        const QString path = QFileDialog::getSaveFileName(
            this,
            "Save Structured Text",
            "program.st",
            "Structured Text (*.st)");
        if (path.isEmpty()) {
            return false;
        }
        setCurrentFile(path);
    }

    QFile file(m_currentFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox box(QMessageBox::Critical, "Save Failed", file.errorString(), QMessageBox::Ok, this);
        styleMessageBox(box);
        box.exec();
        return false;
    }

    if (file.write(m_editor->toPlainText().toUtf8()) < 0) {
        QMessageBox box(QMessageBox::Critical, "Save Failed", file.errorString(), QMessageBox::Ok, this);
        styleMessageBox(box);
        box.exec();
        return false;
    }

    m_dirty = false;
    updateWindowTitle();
    appendLog(QString("Saved %1").arg(m_currentFile));
    saveSettings();
    return true;
}

void MainWindow::setCurrentFile(const QString &path)
{
    m_currentFile = QFileInfo(path).absoluteFilePath();
    updateWindowTitle();
}

void MainWindow::openFilePath(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox box(QMessageBox::Critical, "Open Failed", file.errorString(), QMessageBox::Ok, this);
        styleMessageBox(box);
        box.exec();
        return;
    }

    m_editor->setPlainText(QString::fromUtf8(file.readAll()));
    setCurrentFile(path);
    m_lastPackagePath.clear();
    m_dirty = false;
    updateWindowTitle();
    appendLog(QString("Opened %1").arg(path));
    saveSettings();
}

bool MainWindow::compileCurrent()
{
    saveSettings();

    if (m_currentFile.isEmpty() || m_dirty) {
        if (!saveCurrentFile()) {
            return false;
        }
    }

    const QString outputPath = packagePathForSource(m_currentFile);
    QString combinedOutput;
    appendLog(QString("Compiling %1 -> %2").arg(m_currentFile, outputPath));

    const bool ok = m_compiler.compile(m_currentFile, outputPath, combinedOutput);
    if (!combinedOutput.isEmpty()) {
        appendLog(combinedOutput.trimmed());
    }

    if (!ok) {
        QMessageBox box(QMessageBox::Critical, "Compile Failed", combinedOutput.trimmed(), QMessageBox::Ok, this);
        styleMessageBox(box);
        box.exec();
        return false;
    }

    m_lastPackagePath = outputPath;
    updateWindowTitle();
    appendLog(QString("Compile succeeded (%1 bytes).").arg(QFileInfo(outputPath).size()));
    showInfoDialog("Compile Complete",
                   QString("Compiled %1\n\n%2 bytes written to:\n%3")
                       .arg(QFileInfo(m_currentFile).fileName())
                       .arg(QFileInfo(outputPath).size())
                       .arg(outputPath));
    return true;
}

void MainWindow::discoverDevices()
{
    saveSettings();
    m_discovery.setCredentials(m_userEdit->text(), m_passwordEdit->text());

    DiscoveryDialog dialog(&m_discovery, this);
    if (dialog.exec() == QDialog::Accepted) {
        const QString ip = dialog.selectedIp();
        if (!ip.isEmpty()) {
            m_deviceHostEdit->setText(ip);
            saveSettings();
            appendLog(QString("Selected target %1").arg(ip));
        }
    }

    m_discovery.cancelDiscovery();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    if (m_serialMonitor != nullptr) {
        m_serialMonitor->disconnectPort();
    }
    if (!maybeSave()) {
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}
