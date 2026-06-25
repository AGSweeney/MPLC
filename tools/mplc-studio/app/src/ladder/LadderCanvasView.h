/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QGraphicsView>

class LadderScene;
class QTimer;

class LadderCanvasView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit LadderCanvasView(LadderScene *scene, QWidget *parent = nullptr);

    void resetZoom();
    void zoomIn();
    void zoomOut();
    void updateSceneLayoutWidth();

signals:
    void addContactRequested(bool negated);
    void addCoilRequested();
    void addRungRequested();
    void surroundWithBranchRequested();
    void removeBranchRequested();
    void addParallelBranchLegRequested(bool negated);
    void addContactAtCellRequested(int rungIndex, int row, int col, bool negated);
    void deleteRequested();

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QTimer *m_resizeLayoutTimer = nullptr;
    qreal m_zoom = 1.0;
};
