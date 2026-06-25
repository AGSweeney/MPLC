/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderElementItem.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

LadderElementItem::LadderElementItem(LadderProgram *program, LadderElement *element, QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , m_program(program)
    , m_localId(element != nullptr ? element->localId : 0)
{
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
}

LadderElement *LadderElementItem::element() const
{
    if (m_program == nullptr || m_localId <= 0) {
        return nullptr;
    }
    return m_program->elementById(m_localId);
}

QRectF LadderElementItem::boundingRect() const
{
    return QRectF(0, 0, kCellWidth, kCellHeight);
}

QPointF LadderElementItem::leftPort() const
{
    return QPointF(0, kBusLineY);
}

QPointF LadderElementItem::rightPort() const
{
    return QPointF(kCellWidth, kBusLineY);
}

void LadderElementItem::drawSelection(QPainter *painter, const QRectF &rect) const
{
    if (isSelected()) {
        painter->setPen(QPen(QColor("#0078d7"), 2, Qt::SolidLine));
        painter->setBrush(QColor(0, 120, 215, 35));
        painter->drawRoundedRect(rect.adjusted(2, 2, -2, -2), 4, 4);
    }
}

void LadderElementItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    drawSelection(painter, boundingRect());
}

void LadderElementItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    emit elementDoubleClicked(this);
    QGraphicsItem::mouseDoubleClickEvent(event);
}

void LadderElementItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_dragging = false;
    emit elementActivated(this);
    QGraphicsItem::mousePressEvent(event);
}

void LadderElementItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        emit elementDragFinished(this);
    }
}

void LadderElementItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        m_dragging = true;
    }
    QGraphicsItem::mouseMoveEvent(event);
}
