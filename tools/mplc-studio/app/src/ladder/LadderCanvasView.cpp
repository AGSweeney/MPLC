/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderCanvasView.h"
#include "LadderMime.h"
#include "LadderScene.h"

#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QResizeEvent>
#include <QTimer>
#include <QWheelEvent>
#include <cmath>

namespace {

constexpr qreal kMinZoom = 0.75;
constexpr qreal kMaxZoom = 2.0;

} // namespace

LadderCanvasView::LadderCanvasView(LadderScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
    setObjectName("LadderCanvas");
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::TextAntialiasing, true);
    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setFrameShape(QFrame::NoFrame);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setAcceptDrops(true);
    m_resizeLayoutTimer = new QTimer(this);
    m_resizeLayoutTimer->setSingleShot(true);
    m_resizeLayoutTimer->setInterval(50);
    connect(m_resizeLayoutTimer, &QTimer::timeout, this, &LadderCanvasView::updateSceneLayoutWidth);
    resetZoom();
}

void LadderCanvasView::updateSceneLayoutWidth()
{
    auto *ladderScene = qobject_cast<LadderScene *>(scene());
    if (ladderScene == nullptr) {
        return;
    }
    ladderScene->setLayoutWidth(static_cast<qreal>(viewport()->width()));
}

void LadderCanvasView::resetZoom()
{
    m_zoom = 1.0;
    setTransform(QTransform());
}

void LadderCanvasView::zoomIn()
{
    m_zoom = qMin(kMaxZoom, m_zoom * 1.15);
    scale(1.15, 1.15);
}

void LadderCanvasView::zoomOut()
{
    m_zoom = qMax(kMinZoom, m_zoom / 1.15);
    scale(1.0 / 1.15, 1.0 / 1.15);
}

void LadderCanvasView::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, QColor("#f4f6f9"));

    const int grid = 20;
    const int left = static_cast<int>(std::floor(rect.left())) - (static_cast<int>(std::floor(rect.left())) % grid);
    const int top = static_cast<int>(std::floor(rect.top())) - (static_cast<int>(std::floor(rect.top())) % grid);

    QPen minor(QColor("#e8ebf0"));
    minor.setWidth(1);
    painter->setPen(minor);
    for (int x = left; x < rect.right(); x += grid) {
        painter->drawLine(x, static_cast<int>(rect.top()), x, static_cast<int>(rect.bottom()));
    }
    for (int y = top; y < rect.bottom(); y += grid) {
        painter->drawLine(static_cast<int>(rect.left()), y, static_cast<int>(rect.right()), y);
    }

    QPen major(QColor("#d8dde6"));
    major.setWidth(1);
    painter->setPen(major);
    const int majorGrid = grid * 4;
    const int leftMajor = static_cast<int>(std::floor(rect.left())) - (static_cast<int>(std::floor(rect.left())) % majorGrid);
    const int topMajor = static_cast<int>(std::floor(rect.top())) - (static_cast<int>(std::floor(rect.top())) % majorGrid);
    for (int x = leftMajor; x < rect.right(); x += majorGrid) {
        painter->drawLine(x, static_cast<int>(rect.top()), x, static_cast<int>(rect.bottom()));
    }
    for (int y = topMajor; y < rect.bottom(); y += majorGrid) {
        painter->drawLine(static_cast<int>(rect.left()), y, static_cast<int>(rect.right()), y);
    }
}

void LadderCanvasView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (m_resizeLayoutTimer != nullptr) {
        m_resizeLayoutTimer->start();
    }
}

void LadderCanvasView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}

void LadderCanvasView::contextMenuEvent(QContextMenuEvent *event)
{
    auto *ladderScene = qobject_cast<LadderScene *>(scene());
    int rungIndex = 0;
    int row = 0;
    int col = 1;
    const QPointF scenePos = mapToScene(event->pos());
    const bool mapped = ladderScene != nullptr
                        && ladderScene->mapScenePosToCell(scenePos, &rungIndex, &row, &col);

    QMenu menu(this);
    if (mapped && row > 0) {
        menu.addAction(QStringLiteral("Add NO Contact on Branch Row"), this, [this, rungIndex, row, col]() {
            emit addContactAtCellRequested(rungIndex, row, col, false);
        });
        menu.addAction(QStringLiteral("Add NC Contact on Branch Row"), this, [this, rungIndex, row, col]() {
            emit addContactAtCellRequested(rungIndex, row, col, true);
        });
        menu.addSeparator();
    }
    menu.addAction(QStringLiteral("Add Normally Open Contact"), this, [this]() { emit addContactRequested(false); });
    menu.addAction(QStringLiteral("Add Normally Closed Contact"), this, [this]() { emit addContactRequested(true); });
    menu.addAction(QStringLiteral("Add Output Coil"), this, [this]() { emit addCoilRequested(); });
    menu.addSeparator();
    menu.addAction(QStringLiteral("Add Rung"), this, [this]() { emit addRungRequested(); });
    menu.addAction(QStringLiteral("Surround with Branch"), this, [this]() { emit surroundWithBranchRequested(); });
    menu.addAction(QStringLiteral("Add Parallel Leg (+ contact)"), this, [this]() {
        emit addParallelBranchLegRequested(false);
    });
    menu.addAction(QStringLiteral("Remove Branch"), this, [this]() { emit removeBranchRequested(); });
    menu.addSeparator();
    menu.addAction(QStringLiteral("Delete Selection / Rung"), this, [this]() { emit deleteRequested(); });
    menu.exec(event->globalPos());
}

void LadderCanvasView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(LadderMime::mimeType())) {
        event->acceptProposedAction();
    }
}

void LadderCanvasView::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat(LadderMime::mimeType())) {
        event->acceptProposedAction();
    }
}

void LadderCanvasView::dropEvent(QDropEvent *event)
{
    auto *ladderScene = qobject_cast<LadderScene *>(scene());
    if (ladderScene == nullptr) {
        return;
    }

    const QPointF scenePos = mapToScene(event->position().toPoint());
    if (ladderScene->dropElementPayload(scenePos, event->mimeData())) {
        event->acceptProposedAction();
    }
}

void LadderCanvasView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) {
        emit deleteRequested();
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}
