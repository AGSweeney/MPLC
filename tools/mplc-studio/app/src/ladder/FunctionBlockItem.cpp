/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FunctionBlockItem.h"
#include "LadderModel.h"

#include <QPainter>

FunctionBlockItem::FunctionBlockItem(LadderProgram *program, LadderElement *element, QGraphicsItem *parent)
    : LadderElementItem(program, element, parent)
{
}

void FunctionBlockItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing, true);

    const LadderElement *element = this->element();
    const QString label = element != nullptr ? element->fbTypeName : QString();
    const QString subLabel = element != nullptr ? LadderModel::functionBlockSubLabel(*element) : QString();

    QFont labelFont = painter->font();
    labelFont.setPointSize(9);
    labelFont.setBold(true);
    painter->setFont(labelFont);
    painter->setPen(QColor("#1a1a1a"));
    painter->drawText(QRectF(0, 2, kCellWidth, 18), Qt::AlignHCenter | Qt::AlignVCenter, label);

    const qreal midY = kBusLineY;
    const QRectF rect(8, midY - 20, kCellWidth - 16, 40);

    painter->setPen(QPen(QColor("#2d325a"), 2));
    painter->setBrush(QColor("#f7f9fc"));
    painter->drawRect(rect);

    QFont blockFont = painter->font();
    blockFont.setBold(true);
    blockFont.setPointSize(10);
    painter->setFont(blockFont);
    painter->setPen(Qt::black);
    painter->drawText(rect, Qt::AlignCenter, label);

    if (!subLabel.isEmpty()) {
        QFont subFont = painter->font();
        subFont.setBold(false);
        subFont.setPointSize(8);
        painter->setFont(subFont);
        painter->setPen(QColor("#4a5568"));
        painter->drawText(QRectF(0, kCellHeight - 18, kCellWidth, 14),
                          Qt::AlignHCenter | Qt::AlignVCenter,
                          subLabel);
    }

    painter->setPen(QPen(QColor("#101010"), 2.5));
    painter->drawLine(QPointF(0, midY), QPointF(rect.left(), midY));
    painter->drawLine(QPointF(rect.right(), midY), QPointF(kCellWidth, midY));

    drawSelection(painter, boundingRect());
}
