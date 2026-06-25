/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QGraphicsRectItem>

class LadderScene;

enum class BranchRegionKind {
    Input,
    Output
};

class BranchRegionItem : public QGraphicsRectItem
{
public:
    static constexpr qreal kHandleWidth = 14.0;

    BranchRegionItem(BranchRegionKind kind,
                     int rungIndex,
                     int regionIndex,
                     LadderScene *scene,
                     QGraphicsItem *parent = nullptr);

    BranchRegionKind kind() const { return m_kind; }
    int rungIndex() const { return m_rungIndex; }
    int regionIndex() const { return m_regionIndex; }

    void setBranchSelected(bool selected);
    bool branchSelected() const { return m_branchSelected; }

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    BranchRegionKind m_kind = BranchRegionKind::Input;
    int m_rungIndex = 0;
    int m_regionIndex = 0;
    LadderScene *m_scene = nullptr;
    bool m_branchSelected = false;
    bool m_hovered = false;
};
