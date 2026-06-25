/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CoilItem.h"

#include <QPainter>

CoilItem::CoilItem(LadderProgram *program, LadderElement *element, QGraphicsItem *parent)
    : LadderElementItem(program, element, parent)
{
}

void CoilItem::cycleCoilKind()
{
    LadderElement *element = this->element();
    if (element == nullptr) {
        return;
    }
    switch (element->coilKind) {
    case LadderCoilKind::Normal:
        element->coilKind = LadderCoilKind::Set;
        break;
    case LadderCoilKind::Set:
        element->coilKind = LadderCoilKind::Reset;
        break;
    case LadderCoilKind::Reset:
        element->coilKind = LadderCoilKind::Normal;
        break;
    }
    update();
    emit elementChanged(this);
}

void CoilItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing, true);

    const LadderElement *element = this->element();
    QString suffix;
    if (element != nullptr) {
        switch (element->coilKind) {
        case LadderCoilKind::Set:
            suffix = QStringLiteral(" (S)");
            break;
        case LadderCoilKind::Reset:
            suffix = QStringLiteral(" (R)");
            break;
        case LadderCoilKind::Normal:
        default:
            break;
        }
    }

    const QString label = element != nullptr ? element->variable + suffix : QString();

    const QRectF symbolRect(8, 24, kCellWidth - 16, 44);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(255, 255, 255, 220));
    painter->drawRoundedRect(symbolRect, 3, 3);

    QFont labelFont(QStringLiteral("Segoe UI"), 9, QFont::Bold);
    painter->setFont(labelFont);
    painter->setPen(QColor("#1a1a1a"));
    painter->drawText(QRectF(0, 2, kCellWidth, 20), Qt::AlignHCenter | Qt::AlignVCenter, label);

    const qreal midY = kBusLineY;
    const qreal arcRadius = 14;
    const qreal centerX = kCellWidth / 2.0;
    const QRectF leftArc(centerX - arcRadius * 2 - 2, midY - arcRadius, arcRadius * 2, arcRadius * 2);
    const QRectF rightArc(centerX + 2, midY - arcRadius, arcRadius * 2, arcRadius * 2);

    QPen wirePen(QColor("#101010"), 2.5, Qt::SolidLine, Qt::RoundCap);
    painter->setPen(wirePen);
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(QPointF(0, midY), QPointF(leftArc.left(), midY));
    painter->drawArc(leftArc, 90 * 16, 180 * 16);
    painter->drawArc(rightArc, 90 * 16, -180 * 16);
    painter->drawLine(QPointF(rightArc.right(), midY), QPointF(kCellWidth, midY));

    drawSelection(painter, boundingRect());
}
