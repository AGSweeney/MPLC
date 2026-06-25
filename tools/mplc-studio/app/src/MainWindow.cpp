/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "MainWindow.h"
#include "SerialMonitorWidget.h"

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

namespace {

bool isLadderProjectPath(const QString &path)
{
    return QFileInfo(path).suffix().compare(QStringLiteral("xml"), Qt::CaseInsensitive) == 0;
}

QString combinedOpenFilter()
{
    return QStringLiteral("All Supported (*.st *.xml);;Structured Text (*.st);;Ladder Project (*.xml);;All Files (*)");
}

} // namespace

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

    connect(&m_compiler, &CompilerService::compileStarted, this, [this]() {
        setBackgroundTaskUiEnabled(false);
        if (m_statusLabel != nullptr) {
            m_statusLabel->setText(QStringLiteral("Compiling..."));
        }
    });
    connect(&m_compiler, &CompilerService::compileFinished, this, &MainWindow::handleCompileFinished);

    connect(&m_stImport, &StImportService::importStarted, this, [this]() {
        setBackgroundTaskUiEnabled(false);
        if (m_statusLabel != nullptr) {
            m_statusLabel->setText(QStringLiteral("Importing ST..."));
        }
    });
    connect(&m_stImport, &StImportService::importFinished, this, &MainWindow::handleStImportFinished);
}

void MainWindow::createActions()
{
    m_newAction = new QAction("New ST", this);
    m_newAction->setShortcut(QKeySequence::New);
    connect(m_newAction, &QAction::triggered, this, &MainWindow::newStructuredTextDocument);

    m_newLadderAction = new QAction("New Ladder", this);
    m_newLadderAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_N);
    connect(m_newLadderAction, &QAction::triggered, this, &MainWindow::newLadderDocument);

    m_openAction = new QAction("Open...", this);
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, [this]() {
        if (!maybeSave()) {
            return;
        }
        const QString path = QFileDialog::getOpenFileName(
            this,
            "Open Project",
            m_currentFile.isEmpty() ? QString() : QFileInfo(m_currentFile).absolutePath(),
            combinedOpenFilter());
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
        if (m_activeEditor == nullptr) {
            return;
        }
        const QString path = QFileDialog::getSaveFileName(
            this,
            "Save Project",
            m_currentFile.isEmpty() ? "program.st" : m_currentFile,
            m_activeEditor->suggestedSaveFilter() + ";;All Files (*)");
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

    m_addRungAction = new QAction("Add Rung", this);
    connect(m_addRungAction, &QAction::triggered, this, [this]() {
        if (m_ladderEditor != nullptr && m_ladderEditor->scene() != nullptr) {
            m_ladderEditor->scene()->addRung();
        }
    });

    m_addContactAction = new QAction("Add Contact", this);
    connect(m_addContactAction, &QAction::triggered, this, [this]() {
        if (m_ladderEditor != nullptr && m_ladderEditor->scene() != nullptr) {
            m_ladderEditor->scene()->addContactToSelectedRung(false);
        }
    });

    m_viewGeneratedStAction = new QAction("View Generated ST", this);
    connect(m_viewGeneratedStAction, &QAction::triggered, this, [this]() {
        if (m_ladderEditor != nullptr) {
            m_ladderEditor->showGeneratedSt();
        }
    });

    m_importStToLadderAction = new QAction("Import ST to Ladder...", this);
    m_importStToLadderAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_I);
    connect(m_importStToLadderAction, &QAction::triggered, this, &MainWindow::importStToLadder);

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

    m_stEditor = new StTextEditor(central);
    m_ladderEditor = new LadderEditorWidget(central);
    m_ladderPropertiesPanel = m_ladderEditor->propertiesPanel();
    m_ladderToolbox = new LadderToolbox(central);
    m_ladderToolbox->setScene(m_ladderEditor->scene());

    m_ladderSidePanel = new QWidget(central);
    auto *ladderSideLayout = new QVBoxLayout(m_ladderSidePanel);
    ladderSideLayout->setContentsMargins(0, 0, 0, 0);
    ladderSideLayout->setSpacing(0);
    ladderSideLayout->addWidget(m_ladderToolbox, 1);
    ladderSideLayout->addWidget(m_ladderPropertiesPanel, 0);
    m_ladderPropertiesPanel->setMaximumWidth(QWIDGETSIZE_MAX);

    m_editorStack = new QStackedWidget(central);
    m_editorStack->addWidget(m_stEditor);
    m_editorStack->addWidget(m_ladderEditor);

    m_activeEditor = m_stEditor;
    m_editorStack->setCurrentWidget(m_stEditor);

    connect(m_stEditor, &DocumentEditor::dirtyChanged, this, &MainWindow::onEditorDirtyChanged);
    connect(m_ladderEditor, &DocumentEditor::dirtyChanged, this, &MainWindow::onEditorDirtyChanged);
    connect(m_stEditor, &DocumentEditor::statusMessage, this, &MainWindow::appendLog);
    connect(m_ladderEditor, &DocumentEditor::statusMessage, this, &MainWindow::appendLog);

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
    editorPaneLayout->addWidget(m_editorStack, 1);

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

    m_stToolbox = new StInstructionToolbox(m_stEditor->editor(), central);

    m_sidePanelStack = new QStackedWidget(central);
    m_sidePanelStack->addWidget(m_stToolbox);
    m_sidePanelStack->addWidget(m_ladderSidePanel);
    m_sidePanelStack->setCurrentWidget(m_stToolbox);

    m_workSplitter = new QSplitter(Qt::Horizontal, central);
    m_workSplitter->addWidget(leftColumn);
    m_workSplitter->addWidget(m_sidePanelStack);
    m_workSplitter->setStretchFactor(0, 1);
    m_workSplitter->setStretchFactor(1, 0);
    m_workSplitter->setCollapsible(1, true);
    m_workSplitter->setSizes({860, 240});

    auto *savedToolboxWidth = new int(240);
    connect(m_stToolbox, &StInstructionToolbox::expandedChanged, this, [this, savedToolboxWidth](bool expanded) {
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
    connect(m_ladderToolbox, &LadderToolbox::expandedChanged, this, [this, savedToolboxWidth](bool expanded) {
        applyLadderSidePanelLayout(expanded);
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
}

void MainWindow::createRibbon()
{
    auto *central = centralWidget();
    auto *rootLayout = static_cast<QVBoxLayout *>(central->layout());

    const QColor ribbonIconColor("#1a1a1a");

    m_newButton = makeRibbonCommandButton("New ST", ":/icons/star-plus.svg", this, ribbonIconColor);
    m_newLadderButton = makeRibbonCommandButton("New Ladder", ":/icons/star-plus.svg", this, ribbonIconColor);
    m_openButton = makeRibbonCommandButton("Open", ":/icons/list.svg", this, ribbonIconColor);
    m_saveButton = makeRibbonCommandButton("Save", ":/icons/chevron-up.svg", this, ribbonIconColor);
    m_compileButton = makeRibbonCommandButton("Compile", ":/icons/list.svg", this, ribbonIconColor);
    m_discoverButton = makeRibbonCommandButton("Discover", ":/icons/search.svg", this, ribbonIconColor);
    m_uploadButton = makeRibbonCommandButton("Upload", ":/icons/link.svg", this, ribbonIconColor);
    m_rebootButton = makeRibbonCommandButton("Reboot", ":/icons/unlink.svg", this, ribbonIconColor);
    m_addRungButton = makeRibbonCommandButton("Add Rung", ":/icons/chevron-down.svg", this, ribbonIconColor);
    m_addContactButton = makeRibbonCommandButton("Add Contact", ":/icons/chevron-right.svg", this, ribbonIconColor);
    m_importStToLadderButton = makeRibbonCommandButton("ST to Ladder", ":/icons/link.svg", this, ribbonIconColor);

    auto *helpButton = makeRibbonCommandButton("About", ":/icons/app-icon-64.png", this, ribbonIconColor);
    helpButton->setToolTip("About MPLC Studio");

    connect(m_newButton, &QToolButton::clicked, m_newAction, &QAction::trigger);
    connect(m_newLadderButton, &QToolButton::clicked, m_newLadderAction, &QAction::trigger);
    connect(m_openButton, &QToolButton::clicked, m_openAction, &QAction::trigger);
    connect(m_saveButton, &QToolButton::clicked, m_saveAction, &QAction::trigger);
    connect(m_compileButton, &QToolButton::clicked, m_compileAction, &QAction::trigger);
    connect(m_discoverButton, &QToolButton::clicked, m_discoverAction, &QAction::trigger);
    connect(m_uploadButton, &QToolButton::clicked, m_uploadAction, &QAction::trigger);
    connect(m_rebootButton, &QToolButton::clicked, m_rebootAction, &QAction::trigger);
    connect(m_addRungButton, &QToolButton::clicked, m_addRungAction, &QAction::trigger);
    connect(m_addContactButton, &QToolButton::clicked, m_addContactAction, &QAction::trigger);
    connect(m_importStToLadderButton, &QToolButton::clicked, m_importStToLadderAction, &QAction::trigger);
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
    appMenu->addAction(m_newLadderAction);
    appMenu->addAction(m_openAction);
    appMenu->addAction(m_importStToLadderAction);
    appMenu->addAction(m_saveAction);
    appMenu->addAction(m_saveAsAction);
    appMenu->addSeparator();
    appMenu->addAction(m_compileAction);
    appMenu->addAction(m_addRungAction);
    appMenu->addAction(m_addContactAction);
    appMenu->addAction(m_viewGeneratedStAction);
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
    fileGroup.commands->addWidget(m_newLadderButton);
    fileGroup.commands->addWidget(m_openButton);
    fileGroup.commands->addWidget(m_importStToLadderButton);
    fileGroup.commands->addWidget(m_saveButton);

    const RibbonGroup ladderGroup = makeRibbonGroup(homePage, "Ladder");
    ladderGroup.commands->addWidget(m_addRungButton);
    ladderGroup.commands->addWidget(m_addContactButton);

    const RibbonGroup findGroup = makeRibbonGroup(homePage, "Find");
    findGroup.commands->addWidget(m_discoverButton);

    const RibbonGroup deviceGroup = makeRibbonGroup(homePage, "Device");
    deviceGroup.commands->addWidget(m_compileButton);
    deviceGroup.commands->addWidget(m_uploadButton);
    deviceGroup.commands->addWidget(m_rebootButton);

    homeLayout->addWidget(fileGroup.widget);
    homeLayout->addWidget(ladderGroup.widget);
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
    if (m_newLadderButton != nullptr) {
        m_newLadderButton->setIcon(StudioTheme::renderSvgIcon(":/icons/star-plus.svg", edge, iconColor));
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
    if (m_addRungButton != nullptr) {
        m_addRungButton->setIcon(StudioTheme::renderSvgIcon(":/icons/chevron-down.svg", edge, iconColor));
    }
    if (m_addContactButton != nullptr) {
        m_addContactButton->setIcon(StudioTheme::renderSvgIcon(":/icons/chevron-right.svg", edge, iconColor));
    }
    if (m_importStToLadderButton != nullptr) {
        m_importStToLadderButton->setIcon(StudioTheme::renderSvgIcon(":/icons/link.svg", edge, iconColor));
    }
}

void MainWindow::showHelpDialog()
{
    QMessageBox box(this);
    box.setWindowTitle("About MPLC Studio");
    box.setIconPixmap(QPixmap(":/icons/app-icon-256.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    box.setText("<b>MPLC Studio</b>");
    box.setInformativeText(
        "Edit Structured Text and Ladder Diagram projects, compile in-process to .mplc packages, "
        "deploy to NetBurner MOD54415 targets, and monitor the target UART from the Serial Monitor tab.");
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

    if (m_stToolbox != nullptr) {
        const bool toolboxExpanded = settings.value("ui/toolboxExpanded", true).toBool();
        if (!toolboxExpanded) {
            m_stToolbox->setExpanded(false);
        }
    }

    newLadderDocument();
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
    if (m_stToolbox != nullptr) {
        settings.setValue("ui/toolboxExpanded", m_stToolbox->isExpanded());
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

void MainWindow::onEditorDirtyChanged(bool dirty)
{
    if (sender() == m_activeEditor) {
        m_dirty = dirty;
        updateWindowTitle();
    }
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
    if (m_activeEditor == nullptr) {
        return false;
    }

    if (m_currentFile.isEmpty()) {
        const QString path = QFileDialog::getSaveFileName(
            this,
            "Save Project",
            m_activeEditor->editorKind() == DocumentEditor::Kind::Ladder ? "program.xml" : "program.st",
            m_activeEditor->suggestedSaveFilter() + ";;All Files (*)");
        if (path.isEmpty()) {
            return false;
        }
        setCurrentFile(path);
        activateEditorForPath(path);
    }

    if (!m_activeEditor->saveToPath(m_currentFile)) {
        QMessageBox box(QMessageBox::Critical, "Save Failed", "Could not save the current project.", QMessageBox::Ok, this);
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

void MainWindow::activateEditorForPath(const QString &path)
{
    if (isLadderProjectPath(path)) {
        m_activeEditor = m_ladderEditor;
        m_editorStack->setCurrentWidget(m_ladderEditor);
    } else {
        m_activeEditor = m_stEditor;
        m_editorStack->setCurrentWidget(m_stEditor);
    }
    updateSidePanelForEditor();
}

void MainWindow::applyLadderSidePanelLayout(bool toolboxExpanded)
{
    if (m_ladderSidePanel == nullptr || m_ladderToolbox == nullptr || m_ladderPropertiesPanel == nullptr) {
        return;
    }

    m_ladderPropertiesPanel->setVisible(toolboxExpanded);
    if (toolboxExpanded) {
        m_ladderSidePanel->setMinimumWidth(220);
        m_ladderSidePanel->setMaximumWidth(320);
        m_ladderToolbox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    } else {
        m_ladderSidePanel->setMinimumWidth(24);
        m_ladderSidePanel->setMaximumWidth(24);
        m_ladderToolbox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }
}

void MainWindow::updateSidePanelForEditor()
{
    if (m_activeEditor == m_ladderEditor) {
        m_sidePanelStack->setCurrentWidget(m_ladderSidePanel);
        applyLadderSidePanelLayout(m_ladderToolbox->isExpanded());
    } else {
        m_sidePanelStack->setCurrentWidget(m_stToolbox);
    }
}

void MainWindow::openFilePath(const QString &path)
{
    activateEditorForPath(path);
    if (m_activeEditor == nullptr || !m_activeEditor->loadFromPath(path)) {
        QMessageBox box(QMessageBox::Critical, "Open Failed", "Could not open " + path, QMessageBox::Ok, this);
        styleMessageBox(box);
        box.exec();
        return;
    }

    setCurrentFile(path);
    m_lastPackagePath.clear();
    m_dirty = false;
    updateWindowTitle();
    appendLog(QString("Opened %1").arg(path));
    saveSettings();
}

void MainWindow::newStructuredTextDocument()
{
    if (!maybeSave()) {
        return;
    }
    m_activeEditor = m_stEditor;
    m_editorStack->setCurrentWidget(m_stEditor);
    updateSidePanelForEditor();
    m_stEditor->newDocument();
    m_currentFile.clear();
    m_lastPackagePath.clear();
    m_dirty = m_stEditor->isDirty();
    updateWindowTitle();
    appendLog("New ST document.");
}

void MainWindow::newLadderDocument()
{
    if (!maybeSave()) {
        return;
    }
    m_activeEditor = m_ladderEditor;
    m_editorStack->setCurrentWidget(m_ladderEditor);
    updateSidePanelForEditor();
    m_ladderEditor->newDocument();
    m_currentFile.clear();
    m_lastPackagePath.clear();
    m_dirty = m_ladderEditor->isDirty();
    updateWindowTitle();
    appendLog("New ladder project.");
}

void MainWindow::importStToLadder()
{
    if (m_stImport.isBusy()) {
        appendLog(QStringLiteral("ST import already in progress."));
        return;
    }

    QString source;
    QString suggestedPath = m_currentFile;

    if (m_activeEditor == m_stEditor && m_stEditor != nullptr && m_stEditor->editor() != nullptr) {
        source = m_stEditor->editor()->toPlainText();
    }

    if (source.trimmed().isEmpty()) {
        const QString path = QFileDialog::getOpenFileName(this,
                                                          QStringLiteral("Import Structured Text"),
                                                          suggestedPath,
                                                          QStringLiteral("Structured Text (*.st);;All Files (*)"));
        if (path.isEmpty()) {
            return;
        }
        suggestedPath = path;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            showInfoDialog(QStringLiteral("Import Failed"), file.errorString());
            return;
        }
        source = QString::fromUtf8(file.readAll());
    } else if (!maybeSave()) {
        return;
    }

    m_pendingStImportSuggestedPath = suggestedPath;
    appendLog(QStringLiteral("Importing Structured Text in background..."));
    m_stImport.importAsync(source);
}

void MainWindow::handleStImportFinished(const StLadderImporter::Result &result)
{
    setBackgroundTaskUiEnabled(true);
    if (m_statusLabel != nullptr) {
        m_statusLabel->clear();
    }

    if (!result.ok) {
        QMessageBox box(this);
        box.setIcon(QMessageBox::Warning);
        box.setWindowTitle(QStringLiteral("Import ST to Ladder"));
        box.setText(QStringLiteral("Could not convert Structured Text to ladder logic."));
        if (!result.diagnostics.isEmpty()) {
            box.setDetailedText(result.diagnostics.join('\n'));
        }
        styleMessageBox(box);
        box.exec();
        return;
    }

    if (m_ladderEditor == nullptr) {
        return;
    }

    m_ladderEditor->applyImportedProgram(result.program);
    m_activeEditor = m_ladderEditor;
    m_editorStack->setCurrentWidget(m_ladderEditor);
    updateSidePanelForEditor();
    m_currentFile.clear();
    m_lastPackagePath.clear();
    m_dirty = true;
    updateWindowTitle();

    appendLog(QStringLiteral("Imported Structured Text as ladder project."));
    if (!result.diagnostics.isEmpty()) {
        appendLog(result.diagnostics.join('\n'));
    }

    const QString suggestedPath = m_pendingStImportSuggestedPath;
    m_pendingStImportSuggestedPath.clear();
    if (!suggestedPath.isEmpty() && suggestedPath.endsWith(QStringLiteral(".st"), Qt::CaseInsensitive)) {
        appendLog(QStringLiteral("Save the ladder project as .xml when you are ready."));
    }
}

void MainWindow::cleanupTempCompileFile()
{
    if (!m_tempCompileCleanupPath.isEmpty()) {
        QFile::remove(m_tempCompileCleanupPath);
        m_tempCompileCleanupPath.clear();
    }
}

bool MainWindow::compileCurrent()
{
    saveSettings();

    if (m_activeEditor == nullptr) {
        return false;
    }

    if (m_compiler.isBusy()) {
        appendLog(QStringLiteral("Compile already in progress."));
        return false;
    }

    if (m_currentFile.isEmpty() || m_dirty) {
        if (!saveCurrentFile()) {
            return false;
        }
    }

    cleanupTempCompileFile();
    const QString compileInput = m_activeEditor->compileInputPath(&m_tempCompileCleanupPath);
    if (compileInput.isEmpty()) {
        QMessageBox box(QMessageBox::Critical,
                        "Compile Failed",
                        "Could not prepare a compile input for the current project.",
                        QMessageBox::Ok,
                        this);
        styleMessageBox(box);
        box.exec();
        return false;
    }

    const QString outputPath = packagePathForSource(compileInput);
    appendLog(QString("Compiling %1 -> %2").arg(compileInput, outputPath));
    m_compiler.compileAsync(compileInput, outputPath);
    return true;
}

void MainWindow::handleCompileFinished(const CompileResult &result)
{
    cleanupTempCompileFile();
    setBackgroundTaskUiEnabled(true);
    if (m_statusLabel != nullptr) {
        m_statusLabel->clear();
    }

    if (!result.combinedOutput.isEmpty()) {
        appendLog(result.combinedOutput.trimmed());
    }

    if (!result.success) {
        QMessageBox box(QMessageBox::Critical,
                        "Compile Failed",
                        result.combinedOutput.trimmed(),
                        QMessageBox::Ok,
                        this);
        styleMessageBox(box);
        box.exec();
        return;
    }

    m_lastPackagePath = result.outputPath;
    updateWindowTitle();
    appendLog(QString("Compile succeeded (%1 bytes).").arg(QFileInfo(result.outputPath).size()));
    showInfoDialog("Compile Complete",
                   QString("Compiled %1\n\n%2 bytes written to:\n%3")
                       .arg(QFileInfo(result.sourcePath).fileName())
                       .arg(QFileInfo(result.outputPath).size())
                       .arg(result.outputPath));
}

void MainWindow::setBackgroundTaskUiEnabled(bool enabled)
{
    if (m_compileAction != nullptr) {
        m_compileAction->setEnabled(enabled);
    }
    if (m_compileButton != nullptr) {
        m_compileButton->setEnabled(enabled);
    }
    if (m_importStToLadderAction != nullptr) {
        m_importStToLadderAction->setEnabled(enabled);
    }
    if (m_importStToLadderButton != nullptr) {
        m_importStToLadderButton->setEnabled(enabled);
    }
}

bool MainWindow::waitForBackgroundTasks()
{
    if (!m_compiler.isBusy() && !m_stImport.isBusy()) {
        return true;
    }

    QMessageBox box(QMessageBox::Question,
                    QStringLiteral("Background Task Running"),
                    QStringLiteral("A compile or import is still running. Wait for it to finish?"),
                    QMessageBox::Yes | QMessageBox::No,
                    this);
    styleMessageBox(box);
    if (box.exec() != QMessageBox::Yes) {
        return false;
    }

    m_compiler.waitForIdle();
    m_stImport.waitForIdle();
    setBackgroundTaskUiEnabled(true);
    if (m_statusLabel != nullptr) {
        m_statusLabel->clear();
    }
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
    if (!waitForBackgroundTasks()) {
        event->ignore();
        return;
    }
    cleanupTempCompileFile();
    if (m_serialMonitor != nullptr) {
        m_serialMonitor->disconnectPort();
    }
    if (!maybeSave()) {
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}
