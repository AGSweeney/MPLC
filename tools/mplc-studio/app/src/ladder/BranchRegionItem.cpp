/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "BranchRegionItem.h"
#include "LadderScene.h"

#include <QCursor>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QPainter>

BranchRegionItem::BranchRegionItem(BranchRegionKind kind,
                                   int rungIndex,
                                   int regionIndex,
                                   LadderScene *scene,
                                   QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
    , m_kind(kind)
    , m_rungIndex(rungIndex)
    , m_regionIndex(regionIndex)
    , m_scene(scene)
{
    setPen(Qt::NoPen);
    setZValue(4);
    setAcceptHoverEvents(true);
    setToolTip(m_kind == BranchRegionKind::Input
                   ? QStringLiteral("Input branch — click to select, right-click to add parallel leg")
                   : QStringLiteral("Output branch — click to select, right-click to remove"));
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setCursor(Qt::PointingHandCursor);
}

void BranchRegionItem::setBranchSelected(bool selected)
{
    if (m_branchSelected == selected) {
        return;
    }
    m_branchSelected = selected;
    update();
}

void BranchRegionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    const QRectF r = rect();
    QColor fill(0, 120, 215, m_branchSelected ? 48 : (m_hovered ? 28 : 14));
    painter->fillRect(r, fill);

    QColor border(0, 120, 215, m_branchSelected ? 220 : (m_hovered ? 160 : 100));
    painter->setPen(QPen(border, m_branchSelected ? 2.0 : 1.0));
    painter->drawRect(r);

    painter->setPen(border);
    QFont font(QStringLiteral("Segoe UI"), 7, QFont::Bold);
    painter->setFont(font);
    if (m_kind == BranchRegionKind::Input) {
        painter->drawText(r, Qt::AlignHCenter | Qt::AlignVCenter, QStringLiteral("OR"));
    } else {
        painter->drawText(r.adjusted(0, 2, 0, 0), Qt::AlignHCenter | Qt::AlignTop, QStringLiteral("OUT"));
    }
}

void BranchRegionItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_scene == nullptr) {
        event->ignore();
        return;
    }

    if (event->button() == Qt::RightButton) {
        event->accept();
        return;
    }

    if (m_kind == BranchRegionKind::Input) {
        m_scene->selectInputBranch(m_rungIndex, m_regionIndex);
    } else {
        m_scene->selectOutputBranch(m_rungIndex);
    }
    event->accept();
}

void BranchRegionItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (m_scene == nullptr) {
        event->ignore();
        return;
    }

    if (m_kind == BranchRegionKind::Input) {
        m_scene->selectInputBranch(m_rungIndex, m_regionIndex);
    } else {
        m_scene->selectOutputBranch(m_rungIndex);
    }

    QMenu menu;
    if (m_kind == BranchRegionKind::Input) {
        menu.addAction(QStringLiteral("Add Parallel Leg (NO Contact)"), [this]() {
            if (m_scene != nullptr) {
                m_scene->addParallelBranchLeg(false);
            }
        });
        menu.addAction(QStringLiteral("Add Parallel Leg (NC Contact)"), [this]() {
            if (m_scene != nullptr) {
                m_scene->addParallelBranchLeg(true);
            }
        });
        menu.addSeparator();
    }
    menu.addAction(QStringLiteral("Remove This Branch"), [this]() {
        if (m_scene != nullptr) {
            m_scene->removeBranchFromSelectedRung();
        }
    });
    menu.exec(event->screenPos());
    event->accept();
}

void BranchRegionItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    m_hovered = true;
    update();
    QGraphicsRectItem::hoverEnterEvent(event);
}

void BranchRegionItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    m_hovered = false;
    update();
    QGraphicsRectItem::hoverLeaveEvent(event);
}
