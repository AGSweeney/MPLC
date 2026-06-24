/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "StudioTheme.h"

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QIcon>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QSvgRenderer>

namespace StudioTheme {

QString styleSheet()
{
    return QStringLiteral(
        "* { font-family: 'Segoe UI', 'Arial', sans-serif; font-size: 12px; }"
        "QMainWindow, QDialog { background-color: #d4d4d4; color: #000000; }"
        "QWidget { background-color: #d4d4d4; color: #000000; }"
        "QLabel { color: #000000; }"
        "QLabel#EndpointLabel { color: #000000; font-weight: 600; }"
        "QWidget#AcadRibbon { background-color: #d4d4d4; border: 0; border-bottom: 1px solid #a0a0a0; }"
        "QWidget#RibbonTabRow { background-color: #d4d4d4; border-bottom: 1px solid #a0a0a0; }"
        "QToolButton#ApplicationMenuButton { background-color: #d4d4d4; border: 0; border-radius: 0; padding: 2px 6px; "
        "color: #000000; }"
        "QToolButton#ApplicationMenuButton:hover { background-color: #c8c8c8; }"
        "QToolButton#ApplicationMenuButton:pressed { background-color: #0078d7; color: #ffffff; }"
        "QPushButton#RibbonTabButton { background: transparent; color: #2d325a; border: none; padding: 6px 18px; "
        "min-width: 68px; font-size: 11px; font-weight: 600; }"
        "QPushButton#RibbonTabButton:checked { background-color: #e1e1e1; color: #000000; border-bottom: 2px solid #0078d7; "
        "margin-bottom: -1px; padding-bottom: 4px; }"
        "QPushButton#RibbonTabButton:hover:!checked { background-color: #c8c8c8; color: #000000; }"
        "QStackedWidget#RibbonPages { background-color: #e1e1e1; border-bottom: 1px solid #a0a0a0; }"
        "QWidget#RibbonBar { background-color: #e1e1e1; }"
        "QWidget#RibbonGroup { background: transparent; border: none; border-right: 1px solid #b0b0b0; margin: 4px 0; }"
        "QLabel#RibbonGroupCaption { color: #2d325a; font-size: 10px; font-weight: 600; background: transparent; "
        "padding: 2px 4px; }"
        "QLabel#RibbonInlineLabel { color: #2d325a; font-size: 10px; background: transparent; }"
        "QWidget#RibbonInlinePanel { background: transparent; }"
        "QToolButton#RibbonCommandButton { background-color: #e1e1e1; color: #000000; border: 1px solid transparent; "
        "border-radius: 2px; padding: 6px 10px; min-height: 76px; min-width: 56px; font-size: 11px; }"
        "QToolButton#RibbonCommandButton:hover { background-color: #e8f0f8; border-color: #a0a0a0; }"
        "QToolButton#RibbonCommandButton:pressed { background-color: #0078d7; color: #ffffff; }"
        "QToolButton#RibbonCommandButton:checked { background-color: #0078d7; color: #ffffff; }"
        "QToolButton#RibbonCommandButton:disabled { color: #808080; background-color: #e1e1e1; }"
        "QLineEdit, QSpinBox, QDoubleSpinBox { background-color: #ffffff; color: #000000; border: 1px solid #a0a0a0; "
        "border-radius: 0; padding: 3px 6px; }"
        "QComboBox { background-color: #ffffff; color: #000000; border: 1px solid #a0a0a0; border-radius: 0; "
        "padding: 3px 22px 3px 6px; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox::down-arrow { width: 12px; height: 12px; image: url(:/icons/chevron-down.svg); }"
        "QPlainTextEdit { background-color: #ffffff; color: #000000; border: 1px solid #a0a0a0; "
        "font-family: 'Consolas', 'Courier New', monospace; font-size: 11px; }"
        "QPlainTextEdit#CodeEditor { background-color: #ffffff; color: #000000; border: 1px solid #a0a0a0; "
        "font-family: 'Consolas', 'Courier New', monospace; font-size: 13px; padding: 4px; }"
        "QTableWidget { background-color: #ffffff; alternate-background-color: #eef3f8; color: #000000; "
        "border: 1px solid #a0a0a0; gridline-color: #c8c8c8; }"
        "QTableWidget::item { color: #000000; padding: 2px 4px; }"
        "QTableWidget::item:selected { background-color: #0078d7; color: #ffffff; }"
        "QPushButton { background-color: #e1e1e1; color: #000000; border: 1px solid #a0a0a0; border-radius: 3px; "
        "padding: 5px 12px; }"
        "QPushButton:hover { background-color: #f0f0f0; }"
        "QPushButton:pressed { background-color: #d0d0d0; }"
        "QPushButton:disabled { color: #808080; background-color: #e8e8e8; }"
        "QPushButton#ToolbarButton { background-color: #e8e8e8; color: #000000; border: 1px solid #a0a0a0; "
        "border-radius: 3px; padding: 2px 10px; min-height: 36px; max-height: 36px; font-size: 11px; }"
        "QPushButton#ToolbarButton:hover { background-color: #f0f0f0; border-color: #808080; }"
        "QPushButton#ToolbarButton:pressed { background-color: #d0d0d0; }"
        "QHeaderView::section { background-color: #2d325a; color: #ffffff; padding: 5px 8px; border: none; "
        "border-right: 1px solid #1e2240; border-bottom: 1px solid #1e2240; font-weight: bold; }"
        "QSplitter::handle { background-color: #a0a0a0; }"
        "QSplitter::handle:horizontal { width: 3px; }"
        "QSplitter::handle:vertical { height: 4px; }"
        "QScrollBar:vertical { background: #e8e8e8; width: 14px; margin: 0; border: 1px solid #a0a0a0; }"
        "QScrollBar::handle:vertical { background: #c8c8c8; min-height: 24px; border: 1px solid #a0a0a0; }"
        "QScrollBar::handle:vertical:hover { background: #b0b0b0; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar:horizontal { background: #e8e8e8; height: 14px; margin: 0; border: 1px solid #a0a0a0; }"
        "QScrollBar::handle:horizontal { background: #c8c8c8; min-width: 24px; border: 1px solid #a0a0a0; }"
        "QScrollBar::handle:horizontal:hover { background: #b0b0b0; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
        "QStatusBar { background-color: #2d325a; color: #ffffff; border-top: 1px solid #1e2240; }"
        "QStatusBar::item { border: none; background: transparent; }"
        "QStatusBar QLabel, QLabel#StatusLabel { color: #ffffff; background: transparent; border: none; padding: 0 8px; }"
        "QTabWidget#BottomTabs { background-color: #e1e1e1; border: none; }"
        "QTabWidget#BottomTabs::pane { border: 1px solid #a0a0a0; top: -1px; background-color: #e1e1e1; }"
        "QTabWidget#BottomTabs QTabBar::tab { background-color: #d4d4d4; color: #2d325a; border: 1px solid #a0a0a0; "
        "border-bottom: none; padding: 5px 14px; margin-right: 2px; font-size: 11px; font-weight: 600; }"
        "QTabWidget#BottomTabs QTabBar::tab:selected { background-color: #e1e1e1; color: #000000; border-bottom: 2px solid #0078d7; }"
        "QTabWidget#BottomTabs QTabBar::tab:hover:!selected { background-color: #c8c8c8; }"
        "QWidget#LogToggleBar { background-color: #2d325a; border-top: 2px solid #1e2240; }"
        "QLabel#LogToggleLabel { color: #ffffff; font-size: 11px; font-weight: bold; background: transparent; }"
        "QPushButton#LogToggleButton { color: #ffffff; background: #3d4570; border: 1px solid #5a6290; "
        "border-radius: 2px; padding: 0 10px; font-size: 11px; }"
        "QPushButton#LogToggleButton:hover { background-color: #4a5488; }"
        "QWidget#StToolbox { background-color: #e1e1e1; border-left: 1px solid #a0a0a0; }"
        "QWidget#StToolboxCollapsedTab { background-color: #d4d4d4; border-left: 1px solid #a0a0a0; }"
        "QWidget#StToolboxCollapsedTab:hover { background-color: #c8c8c8; }"
        "QWidget#StToolboxHeader { background-color: #2d325a; border-bottom: 1px solid #1e2240; }"
        "QLabel#StToolboxTitle { color: #ffffff; font-size: 12px; font-weight: 600; background: transparent; }"
        "QLabel#StToolboxSubtitle { color: #b8c0e0; font-size: 10px; background: transparent; padding-top: 1px; }"
        "QPushButton#StToolboxToggle { color: #ffffff; background: transparent; border: none; border-radius: 2px; "
        "padding: 0; }"
        "QPushButton#StToolboxToggle:hover { background-color: #3d4570; }"
        "QWidget#StToolboxSearchBar { background-color: #d4d4d4; border-bottom: 1px solid #a0a0a0; }"
        "QLabel#StToolboxSearchIcon { background: transparent; }"
        "QLineEdit#StToolboxSearch { background-color: #ffffff; border: 1px solid #a0a0a0; border-radius: 2px; "
        "padding: 4px 6px; font-size: 11px; min-height: 22px; }"
        "QLineEdit#StToolboxSearch:focus { border-color: #0078d7; }"
        "QWidget#StToolboxPanel { background-color: #e1e1e1; }"
        "QScrollArea#StToolboxScroll { background-color: #e1e1e1; border: none; }"
        "QWidget#StToolboxGroup { background-color: #ffffff; border: 1px solid #c8c8c8; border-radius: 3px; }"
        "QToolButton#StToolboxGroupHeader { background-color: #f5f5f5; color: #2d325a; border: none; "
        "border-bottom: 1px solid #e0e0e0; padding: 8px 10px; font-size: 12px; font-weight: 600; "
        "text-align: left; min-height: 32px; }"
        "QToolButton#StToolboxGroupHeader:hover { background-color: #e8f0f8; }"
        "QToolButton#StToolboxGroupHeader:checked { background-color: #f5f5f5; }"
        "QWidget#StToolboxGroupBody { background-color: #ffffff; }"
        "QPushButton#StToolboxSnippet { background-color: #ffffff; color: #000000; border: 1px solid #c8c8c8; "
        "border-radius: 3px; padding: 10px 12px; text-align: left; font-size: 12px; font-weight: 600; "
        "min-height: 40px; }"
        "QPushButton#StToolboxSnippet:hover { background-color: #e8f0f8; border-color: #0078d7; }"
        "QPushButton#StToolboxSnippet:pressed { background-color: #cce4f7; border-color: #0078d7; }"
        "QCheckBox { color: #000000; spacing: 6px; }"
        "QMessageBox { background-color: #d4d4d4; color: #000000; }"
        "QMessageBox QLabel { color: #000000; background-color: transparent; }"
        "QMessageBox QPushButton { background-color: #e1e1e1; color: #000000; border: 1px solid #a0a0a0; "
        "border-radius: 3px; padding: 5px 16px; min-width: 72px; }"
        "QMessageBox QPushButton:hover { background-color: #f0f0f0; }"
        "QMessageBox QPushButton:pressed { background-color: #d0d0d0; }");
}

QString dialogStyleSheet()
{
    return QStringLiteral(
        "QDialog { background-color: #d4d4d4; color: #000000; }"
        "QLabel { color: #000000; background: transparent; }"
        "QLineEdit, QComboBox { background-color: #ffffff; color: #000000; border: 1px solid #a0a0a0; }"
        "QPushButton { background-color: #e1e1e1; color: #000000; border: 1px solid #a0a0a0; "
        "border-radius: 3px; padding: 5px 12px; }"
        "QPushButton:hover { background-color: #f0f0f0; }"
        "QPushButton:pressed { background-color: #d0d0d0; }");
}

void apply(QWidget *root)
{
    const QColor windowColor("#d4d4d4");
    const QColor baseColor("#ffffff");
    const QColor alternateBaseColor("#eef3f8");
    const QColor textColor("#000000");
    const QColor disabledTextColor("#808080");
    const QColor highlightColor("#0078d7");
    const QColor highlightedTextColor("#ffffff");

    QPalette pal;
    pal.setColor(QPalette::Window, windowColor);
    pal.setColor(QPalette::WindowText, textColor);
    pal.setColor(QPalette::Base, baseColor);
    pal.setColor(QPalette::AlternateBase, alternateBaseColor);
    pal.setColor(QPalette::Text, textColor);
    pal.setColor(QPalette::Button, QColor("#e1e1e1"));
    pal.setColor(QPalette::ButtonText, textColor);
    pal.setColor(QPalette::BrightText, highlightedTextColor);
    pal.setColor(QPalette::Highlight, highlightColor);
    pal.setColor(QPalette::HighlightedText, highlightedTextColor);
    pal.setColor(QPalette::ToolTipBase, baseColor);
    pal.setColor(QPalette::ToolTipText, textColor);
    pal.setColor(QPalette::Link, highlightColor);
    pal.setColor(QPalette::Disabled, QPalette::WindowText, disabledTextColor);
    pal.setColor(QPalette::Disabled, QPalette::Text, disabledTextColor);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, disabledTextColor);
    pal.setColor(QPalette::Inactive, QPalette::Highlight, highlightColor);
    pal.setColor(QPalette::Inactive, QPalette::HighlightedText, highlightedTextColor);

    if (qApp != nullptr) {
        qApp->setPalette(pal);
    }
    if (root != nullptr) {
        root->setPalette(pal);
        root->setStyleSheet(styleSheet());
    }
}

void styleLogView(QPlainTextEdit *logView)
{
    if (logView == nullptr) {
        return;
    }
    QPalette pal = logView->palette();
    pal.setColor(QPalette::Base, QColor(0x2d, 0x32, 0x5a));
    pal.setColor(QPalette::Text, QColor(0xff, 0xff, 0xff));
    logView->setPalette(pal);
    logView->setFont(QFont(QStringLiteral("Consolas"), 10));
}

void styleMessageBox(QMessageBox &box)
{
    const QColor windowColor("#d4d4d4");
    const QColor textColor("#000000");
    const QColor buttonColor("#e1e1e1");

    QPalette palette = box.palette();
    palette.setColor(QPalette::Window, windowColor);
    palette.setColor(QPalette::WindowText, textColor);
    palette.setColor(QPalette::Text, textColor);
    palette.setColor(QPalette::Button, buttonColor);
    palette.setColor(QPalette::ButtonText, textColor);
    box.setPalette(palette);
    box.setStyleSheet(dialogStyleSheet());
}

QIcon renderSvgIcon(const QString &resourcePath, int edge, const QColor &tint)
{
    QSvgRenderer renderer(resourcePath);
    if (!renderer.isValid()) {
        return QIcon(resourcePath);
    }
    QPixmap pixmap(edge, edge);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();
    if (tint.isValid()) {
        QPainter tintPainter(&pixmap);
        tintPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        tintPainter.fillRect(pixmap.rect(), tint);
        tintPainter.end();
    }
    const QIcon icon(pixmap);
    if (icon.isNull()) {
        return QIcon(resourcePath);
    }
    return icon;
}

} // namespace StudioTheme
