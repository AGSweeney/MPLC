/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "WireItem.h"

#include <QGraphicsScene>
#include <QPen>

WireItem::WireItem(QGraphicsItem *parent)
    : QGraphicsLineItem(parent)
{
    setPen(QPen(QColor("#101010"), 2.5, Qt::SolidLine, Qt::RoundCap));
    setZValue(-1);
}

WireItem *WireItem::addSegment(QGraphicsScene *scene, const QLineF &line)
{
    if (scene == nullptr || line.length() < 0.5) {
        return nullptr;
    }

    auto *wire = new WireItem();
    wire->setLine(line);
    scene->addItem(wire);
    return wire;
}
