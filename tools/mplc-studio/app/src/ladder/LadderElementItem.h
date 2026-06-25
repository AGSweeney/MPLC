/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderModel.h"

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QObject>

class LadderElementItem : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    static constexpr int kCellWidth = 80;
    static constexpr int kCellHeight = 88;
    static constexpr qreal kBusLineY = kCellHeight / 2.0;

    explicit LadderElementItem(LadderProgram *program, LadderElement *element, QGraphicsItem *parent = nullptr);

    LadderElement *element() const;
    int localId() const { return m_localId; }

    QRectF boundingRect() const override;
    QPointF leftPort() const;
    QPointF rightPort() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

signals:
    void elementChanged(LadderElementItem *item);
    void elementActivated(LadderElementItem *item);
    void elementDoubleClicked(LadderElementItem *item);
    void elementDragFinished(LadderElementItem *item);

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    void drawSelection(QPainter *painter, const QRectF &rect) const;

    LadderProgram *m_program = nullptr;
    int m_localId = 0;
    bool m_dragging = false;
};
