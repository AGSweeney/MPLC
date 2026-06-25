/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QGraphicsRectItem>

class PowerRailItem : public QGraphicsRectItem
{
public:
    explicit PowerRailItem(bool leftRail, qreal height, QGraphicsItem *parent = nullptr);
};
