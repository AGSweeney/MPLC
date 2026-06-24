/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QColor>
#include <QIcon>
#include <QString>

class QMessageBox;
class QPlainTextEdit;
class QWidget;

namespace StudioTheme {

QString styleSheet();
QString dialogStyleSheet();
void apply(QWidget *root);
void styleLogView(QPlainTextEdit *logView);
void styleMessageBox(QMessageBox &box);
QIcon renderSvgIcon(const QString &resourcePath, int edge = 18, const QColor &tint = QColor());

} // namespace StudioTheme
