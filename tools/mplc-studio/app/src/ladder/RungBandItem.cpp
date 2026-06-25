/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "RungBandItem.h"
#include "LadderScene.h"

#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

RungBandItem::RungBandItem(int rungIndex, LadderScene *scene, QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
    , m_rungIndex(rungIndex)
    , m_scene(scene)
{
    setPen(Qt::NoPen);
    setZValue(-10);
    setAcceptHoverEvents(true);
    setCursor(Qt::PointingHandCursor);
}

void RungBandItem::setHighlighted(bool highlighted)
{
    if (m_highlighted == highlighted) {
        return;
    }
    m_highlighted = highlighted;
    update();
}

void RungBandItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (m_highlighted) {
        painter->fillRect(rect(), QColor(0, 120, 215, 22));
        painter->setPen(QPen(QColor("#0078d7"), 1));
        painter->drawRect(rect());
        return;
    }

    if (m_hovered) {
        painter->fillRect(rect(), QColor(0, 0, 0, 10));
        return;
    }

    if (m_rungIndex % 2 == 0) {
        painter->fillRect(rect(), QColor(255, 255, 255, 50));
    }
}

void RungBandItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    m_hovered = true;
    update();
    QGraphicsRectItem::hoverEnterEvent(event);
}

void RungBandItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    m_hovered = false;
    update();
    QGraphicsRectItem::hoverLeaveEvent(event);
}

void RungBandItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_scene != nullptr) {
        m_scene->selectRung(m_rungIndex);
    }
    event->accept();
}
