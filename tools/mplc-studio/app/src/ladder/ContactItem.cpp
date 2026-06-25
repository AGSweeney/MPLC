/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ContactItem.h"

#include <QPainter>

ContactItem::ContactItem(LadderProgram *program, LadderElement *element, QGraphicsItem *parent)
    : LadderElementItem(program, element, parent)
{
}

void ContactItem::toggleNegated()
{
    if (LadderElement *element = this->element()) {
        element->negated = !element->negated;
        update();
        emit elementChanged(this);
    }
}

void ContactItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing, true);

    const LadderElement *element = this->element();
    const QString label = element != nullptr ? element->variable : QString();
    const bool negated = element != nullptr && element->negated;

    const QRectF symbolRect(8, 24, kCellWidth - 16, 44);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(255, 255, 255, 220));
    painter->drawRoundedRect(symbolRect, 3, 3);

    QFont labelFont(QStringLiteral("Segoe UI"), 9, QFont::Bold);
    painter->setFont(labelFont);
    painter->setPen(QColor("#1a1a1a"));
    painter->drawText(QRectF(0, 2, kCellWidth, 20), Qt::AlignHCenter | Qt::AlignVCenter, label);

    const qreal midY = kBusLineY;
    const qreal leftX = 28;
    const qreal rightX = kCellWidth - 28;

    QPen wirePen(QColor("#101010"), 2.5, Qt::SolidLine, Qt::RoundCap);
    painter->setPen(wirePen);
    painter->drawLine(QPointF(0, midY), QPointF(leftX, midY));
    painter->drawLine(QPointF(leftX, midY - 16), QPointF(leftX, midY + 16));
    painter->drawLine(QPointF(rightX, midY - 16), QPointF(rightX, midY + 16));
    painter->drawLine(QPointF(rightX, midY), QPointF(kCellWidth, midY));

    if (negated) {
        QPen slashPen(QColor("#c0392b"), 2.5, Qt::SolidLine, Qt::RoundCap);
        painter->setPen(slashPen);
        painter->drawLine(QPointF(leftX - 2, midY + 16), QPointF(rightX + 2, midY - 16));
        painter->setPen(QColor("#8b0000"));
        painter->drawText(QRectF(0, kCellHeight - 18, kCellWidth, 14), Qt::AlignHCenter, QStringLiteral("/"));
    }

    drawSelection(painter, boundingRect());
}
