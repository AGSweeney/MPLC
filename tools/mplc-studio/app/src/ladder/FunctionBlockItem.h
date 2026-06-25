/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderElementItem.h"

class FunctionBlockItem : public LadderElementItem
{
public:
    explicit FunctionBlockItem(LadderProgram *program, LadderElement *element, QGraphicsItem *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};
