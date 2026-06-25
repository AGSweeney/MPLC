/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "PowerRailItem.h"

#include <QPen>

PowerRailItem::PowerRailItem(bool leftRail, qreal height, QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
{
    Q_UNUSED(leftRail);
    setRect(0, 0, 6, height);
    setPen(QPen(Qt::black, 4));
    setBrush(Qt::NoBrush);
    setZValue(-2);
}
