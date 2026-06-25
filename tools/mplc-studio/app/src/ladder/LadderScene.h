/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderModel.h"

#include "LadderLayout.h"

#include <QGraphicsScene>
#include <QMap>
#include <QMimeData>
#include <QPointF>

class LadderElementItem;
class WireItem;

class LadderScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit LadderScene(QObject *parent = nullptr);

    void setProgram(LadderProgram *program);
    void setLayoutWidth(qreal width);
    void rebuild();
    void addRung();
    void removeSelectedElements();
    void removeSelectedRung();
    void handleDeleteKey();
    void addContactToSelectedRung(bool negated = false);
    void addCoilToSelectedRung(LadderCoilKind kind = LadderCoilKind::Normal);
    void addOutputBranchCoilToSelectedRung(LadderCoilKind kind = LadderCoilKind::Normal);
    void addFunctionBlockToSelectedRung(const QString &typeName);
    void surroundSelectionWithBranch();
    void removeBranchFromSelectedRung();
    void addParallelBranchLeg(bool negated = false);
    void addContactAtCell(int rungIndex, int row, int col, bool negated);
    bool mapScenePosToCell(const QPointF &pos, int *rungIndex, int *row, int *col) const;
    void selectInputBranch(int rungIndex, int regionIndex);
    void selectOutputBranch(int rungIndex);
    void clearBranchSelection();
    void selectRung(int index);
    int selectedRungIndex() const { return m_selectedRungIndex; }
    LadderElementItem *elementItemByLocalId(int localId) const;
    void refreshElementItem(int localId);
    void refreshAllElementItems();

    bool dropElementPayload(const QPointF &scenePos, const QMimeData *mimeData);
    void commitElementDrag(LadderElementItem *item);

signals:
    void programModified();
    void statusMessage(const QString &message);
    void elementActivated(LadderElementItem *item);
    void elementDoubleClicked(LadderElementItem *item);
    void layoutCompleted();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    struct RungMetrics {
        int maxRow = 0;
        int maxCol = 1;
    };

    struct RungLayoutInfo {
        int rungIndex = 0;
        qreal yOffset = 0.0;
        qreal height = 0.0;
    };

    void clearScene();
    void clearDecorations();
    void relayoutForWidth();
    LadderElementItem *createItemForElement(LadderElement *element, int rungIndex);
    RungMetrics rungMetrics(const LadderRung &rung) const;
    qreal effectiveLayoutWidth(const RungMetrics &metrics) const;
    qreal rightRailXForMetrics(const RungMetrics &metrics) const;
    void layoutRung(int rungIndex, qreal yOffset, qreal rightRailX, bool reuseItems = false);
    void drawRungWires(const LadderRung &rung,
                       const QMap<int, LadderElementItem *> &items,
                       qreal yOffset,
                       int maxCol,
                       qreal rightRailX);
    void layoutBranchRegions(int rungIndex, qreal yOffset);
    void updateBranchSelectionVisuals();
    void insertElementBeforeCoil(LadderRung &rung, int row, const std::function<void(int col)> &insert);
    void insertElementAt(int rungIndex, int row, int col, const std::function<void(int col)> &insert);
    void insertElementAfterSelectionOrBeforeCoil(LadderElementKind kind,
                                                 const std::function<void(int row, int col)> &insert);
    void refreshSelectedRung();
    void updateRungHighlights();
    int outputCoilColumn() const;
    void normalizeProgramLayout(int rungIndex = -1);
    int rungIndexForElement(int localId) const;
    bool mapPointToCell(const QPointF &pos, int *rungIndex, int *row, int *col) const;
    void moveElementToCell(int localId, int rungIndex, int row, int col);
    bool parseDropPayload(const QMimeData *mimeData, LadderElementKind *kind, bool *negated,
                          LadderCoilKind *coilKind, QString *fbType) const;
    void placeDroppedElement(int rungIndex, int row, int col, LadderElementKind kind, bool negated,
                             LadderCoilKind coilKind, const QString &fbType);

    LadderProgram *m_program = nullptr;
    QMap<int, LadderElementItem *> m_items;
    QVector<WireItem *> m_wires;
    QVector<class RungBandItem *> m_rungBands;
    QVector<class BranchRegionItem *> m_branchItems;
    QVector<RungLayoutInfo> m_rungLayouts;
    qreal m_layoutWidth = LadderLayout::kMinLayoutWidth;
    qreal m_contentWidth = LadderLayout::kMinLayoutWidth;
    int m_selectedRungIndex = 0;
    int m_selectedBranchRungIndex = -1;
    int m_selectedInputBranchRegionIndex = -1;
    bool m_outputBranchSelected = false;
};
