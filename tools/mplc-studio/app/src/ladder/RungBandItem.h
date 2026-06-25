/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QGraphicsRectItem>

class LadderScene;

class RungBandItem : public QGraphicsRectItem
{
public:
    RungBandItem(int rungIndex, LadderScene *scene, QGraphicsItem *parent = nullptr);

    int rungIndex() const { return m_rungIndex; }
    void setHighlighted(bool highlighted);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    int m_rungIndex = 0;
    LadderScene *m_scene = nullptr;
    bool m_highlighted = false;
    bool m_hovered = false;
};
