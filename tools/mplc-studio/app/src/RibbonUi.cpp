/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "RibbonUi.h"

#include "StudioTheme.h"

RibbonGroup makeRibbonGroup(QWidget *parent, const QString &caption)
{
    auto *group = new QWidget(parent);
    group->setObjectName("RibbonGroup");
    auto *layout = new QVBoxLayout(group);
    layout->setContentsMargins(4, 0, 8, 0);
    layout->setSpacing(2);

    auto *commandsHost = new QWidget(group);
    commandsHost->setObjectName("RibbonGroupBody");
    auto *commands = new QHBoxLayout(commandsHost);
    commands->setContentsMargins(0, 0, 0, 0);
    commands->setSpacing(4);
    commands->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    auto *captionLabel = new QLabel(caption, group);
    captionLabel->setObjectName("RibbonGroupCaption");
    captionLabel->setAlignment(Qt::AlignHCenter);
    captionLabel->setFixedHeight(16);

    layout->addWidget(commandsHost, 1);
    layout->addWidget(captionLabel, 0);
    return {group, commands};
}

QPushButton *makeRibbonTabButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName("RibbonTabButton");
    button->setFlat(true);
    button->setCheckable(true);
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

QToolButton *makeRibbonCommandButton(const QString &text,
                                     const QString &svgPath,
                                     QWidget *parent,
                                     const QColor &iconColor)
{
    auto *btn = new QToolButton(parent);
    btn->setObjectName("RibbonCommandButton");
    btn->setText(text);
    btn->setIcon(StudioTheme::renderSvgIcon(svgPath, 36, iconColor));
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btn->setAutoRaise(false);
    btn->setIconSize(QSize(32, 32));
    btn->setMinimumSize(72, 76);
    return btn;
}
