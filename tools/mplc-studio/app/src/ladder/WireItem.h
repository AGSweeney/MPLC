/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QGraphicsLineItem>

class WireItem : public QGraphicsLineItem
{
public:
    explicit WireItem(QGraphicsItem *parent = nullptr);

    static WireItem *addSegment(QGraphicsScene *scene, const QLineF &line);
};
