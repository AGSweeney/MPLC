/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

struct RibbonGroup {
    QWidget *widget = nullptr;
    QHBoxLayout *commands = nullptr;
};

RibbonGroup makeRibbonGroup(QWidget *parent, const QString &caption);
QPushButton *makeRibbonTabButton(const QString &text, QWidget *parent);
QToolButton *makeRibbonCommandButton(const QString &text,
                                     const QString &svgPath,
                                     QWidget *parent,
                                     const QColor &iconColor);
