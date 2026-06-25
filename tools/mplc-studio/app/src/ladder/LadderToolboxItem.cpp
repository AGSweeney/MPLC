/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderToolboxItem.h"
#include "LadderMime.h"

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>

LadderToolboxItem::LadderToolboxItem(const QIcon &icon, const QString &label, const QString &payload, QWidget *parent)
    : QToolButton(parent)
    , m_payload(payload)
{
    setObjectName(QStringLiteral("LadderToolboxItem"));
    setAutoRaise(false);
    setCursor(Qt::PointingHandCursor);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setIcon(icon);
    setText(label);
    setIconSize(QSize(52, 26));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(34);
    setMaximumHeight(34);
}

void LadderToolboxItem::mousePressEvent(QMouseEvent *event)
{
    m_pressPos = event->pos();
    QToolButton::mousePressEvent(event);
}

void LadderToolboxItem::mouseMoveEvent(QMouseEvent *event)
{
    if (m_payload.isEmpty()) {
        return;
    }
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    if ((event->pos() - m_pressPos).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    auto *drag = new QDrag(this);
    auto *mime = new QMimeData();
    mime->setData(LadderMime::mimeType(), m_payload.toUtf8());
    drag->setMimeData(mime);
    drag->setPixmap(icon().pixmap(iconSize()));
    drag->exec(Qt::CopyAction);
}
