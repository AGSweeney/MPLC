/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderScene.h"
#include "BranchRegionItem.h"
#include "ContactItem.h"
#include "CoilItem.h"
#include "FunctionBlockItem.h"
#include "LadderBranchLogic.h"
#include "LadderIoCatalog.h"
#include "LadderLayout.h"
#include "LadderMime.h"
#include "RungBandItem.h"
#include "WireItem.h"

#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QKeyEvent>
#include <QSet>
#include <algorithm>
#include <functional>

namespace {

QVector<const LadderElement *> elementsOnRow(const LadderRung &rung, int row)
{
    QVector<const LadderElement *> result;
    for (const LadderElement &element : rung.elements) {
        if (element.row == row) {
            result.push_back(&element);
        }
    }
    std::sort(result.begin(), result.end(), [](const LadderElement *a, const LadderElement *b) {
        return a->col < b->col;
    });
    return result;
}

} // namespace

LadderScene::LadderScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setSceneRect(0, 0, LadderLayout::kMinLayoutWidth, 1200);
}

void LadderScene::setProgram(LadderProgram *program)
{
    m_program = program;
    rebuild();
}

void LadderScene::setLayoutWidth(qreal width)
{
    const qreal clamped = qMax(LadderLayout::kMinLayoutWidth, width);
    if (qAbs(m_layoutWidth - clamped) < 16.0) {
        return;
    }
    m_layoutWidth = clamped;
    if (m_items.isEmpty()) {
        rebuild();
        return;
    }
    relayoutForWidth();
}

void LadderScene::clearScene()
{
    clear();
    m_items.clear();
    m_wires.clear();
    m_rungBands.clear();
    m_branchItems.clear();
    m_rungLayouts.clear();
}

void LadderScene::clearDecorations()
{
    QSet<QGraphicsItem *> keep;
    for (LadderElementItem *item : m_items) {
        if (item != nullptr) {
            keep.insert(item);
        }
    }

    const QList<QGraphicsItem *> allItems = items();
    for (QGraphicsItem *item : allItems) {
        if (!keep.contains(item)) {
            removeItem(item);
            delete item;
        }
    }

    m_wires.clear();
    m_rungBands.clear();
    m_branchItems.clear();
    m_rungLayouts.clear();
}

void LadderScene::clearBranchSelection()
{
    m_selectedBranchRungIndex = -1;
    m_selectedInputBranchRegionIndex = -1;
    m_outputBranchSelected = false;
    updateBranchSelectionVisuals();
}

void LadderScene::selectInputBranch(int rungIndex, int regionIndex)
{
    clearSelection();
    m_selectedRungIndex = rungIndex;
    m_selectedBranchRungIndex = rungIndex;
    m_selectedInputBranchRegionIndex = regionIndex;
    m_outputBranchSelected = false;
    updateRungHighlights();
    updateBranchSelectionVisuals();
}

void LadderScene::selectOutputBranch(int rungIndex)
{
    clearSelection();
    m_selectedRungIndex = rungIndex;
    m_selectedBranchRungIndex = rungIndex;
    m_selectedInputBranchRegionIndex = -1;
    m_outputBranchSelected = true;
    updateRungHighlights();
    updateBranchSelectionVisuals();
}

void LadderScene::updateBranchSelectionVisuals()
{
    for (BranchRegionItem *item : m_branchItems) {
        if (item == nullptr) {
            continue;
        }
        const bool selected = item->rungIndex() == m_selectedBranchRungIndex
                              && ((item->kind() == BranchRegionKind::Input
                                   && item->regionIndex() == m_selectedInputBranchRegionIndex)
                                  || (item->kind() == BranchRegionKind::Output && m_outputBranchSelected));
        item->setBranchSelected(selected);
    }
}

void LadderScene::layoutBranchRegions(int rungIndex, qreal yOffset)
{
    if (m_program == nullptr) {
        return;
    }

    LadderRung *rung = m_program->rungAt(rungIndex);
    if (rung == nullptr) {
        return;
    }

    rung->ensureInputBranchRegions();

    for (int i = 0; i < rung->inputBranchRegions.size(); ++i) {
        const LadderInputBranchRegion &region = rung->inputBranchRegions[i];
        const int maxRow = LadderBranchLogic::maxParallelRowForRegion(*rung, region);
        const qreal railInset = region.pathRow * 5.0;
        const qreal left = LadderLayout::columnX(region.forkCol) - BranchRegionItem::kHandleWidth - railInset;

        auto *item = new BranchRegionItem(BranchRegionKind::Input, rungIndex, i, this);
        if (maxRow > region.pathRow) {
            // OR splits downward onto parallel rows only — carrier row stays series (AND).
            const qreal top = yOffset + (region.pathRow + 1) * LadderElementItem::kCellHeight;
            const qreal height = (maxRow - region.pathRow) * LadderElementItem::kCellHeight;
            item->setRect(left, top, BranchRegionItem::kHandleWidth, height);
        } else {
            // No parallel legs yet: affordance below the carrier bus (branch down).
            const qreal busY = LadderLayout::rowBusY(yOffset, region.pathRow);
            item->setRect(left, busY + 6.0, BranchRegionItem::kHandleWidth,
                          LadderElementItem::kCellHeight * 0.35);
        }
        addItem(item);
        m_branchItems.push_back(item);
    }

    int outputForkCol = 0;
    if (rung->hasOutputBranch() && rung->outputBranchForkColumn(&outputForkCol)) {
        int maxCoilRow = 0;
        for (const LadderElement &element : rung->elements) {
            if (element.kind == LadderElementKind::Coil) {
                maxCoilRow = qMax(maxCoilRow, element.row);
            }
        }

        const qreal left = LadderLayout::columnX(outputForkCol) - BranchRegionItem::kHandleWidth;
        auto *item = new BranchRegionItem(BranchRegionKind::Output, rungIndex, -1, this);
        if (maxCoilRow > 0) {
            const qreal top = yOffset + LadderElementItem::kCellHeight;
            const qreal height = maxCoilRow * LadderElementItem::kCellHeight;
            item->setRect(left, top, BranchRegionItem::kHandleWidth, height);
        } else {
            const qreal busY = LadderLayout::rowBusY(yOffset, 0);
            item->setRect(left, busY + 6.0, BranchRegionItem::kHandleWidth,
                          LadderElementItem::kCellHeight * 0.35);
        }
        addItem(item);
        m_branchItems.push_back(item);
    }
}

void LadderScene::relayoutForWidth()
{
    if (m_program == nullptr) {
        return;
    }

    clearDecorations();
    normalizeProgramLayout();

    RungMetrics overallMetrics;
    for (const LadderRung &rung : m_program->rungs) {
        const RungMetrics metrics = rungMetrics(rung);
        overallMetrics.maxRow = qMax(overallMetrics.maxRow, metrics.maxRow);
        overallMetrics.maxCol = qMax(overallMetrics.maxCol, metrics.maxCol);
    }

    m_contentWidth = LadderLayout::columnX(overallMetrics.maxCol) + LadderElementItem::kCellWidth
                     + LadderLayout::kRightRailPad;
    const qreal rightRailX = rightRailXForMetrics(overallMetrics);

    qreal y = LadderLayout::kTopMargin;
    qreal contentBottom = y;

    for (int i = 0; i < m_program->rungs.size(); ++i) {
        layoutRung(i, y, rightRailX, true);
        const RungMetrics metrics = rungMetrics(m_program->rungs[i]);
        contentBottom = y + (metrics.maxRow + 1) * LadderElementItem::kCellHeight;
        y = contentBottom + LadderLayout::kRungSpacing;
    }

    if (contentBottom > LadderLayout::kTopMargin) {
        auto *leftRail = new QGraphicsLineItem(LadderLayout::kLeftRailX,
                                               LadderLayout::kTopMargin,
                                               LadderLayout::kLeftRailX,
                                               contentBottom);
        leftRail->setPen(QPen(QColor("#101010"), 3.0));
        leftRail->setZValue(-3);
        addItem(leftRail);
    }

    const qreal sceneWidth = qMax(m_layoutWidth, m_contentWidth);
    setSceneRect(0, 0, sceneWidth, qMax<qreal>(600.0, contentBottom + 80.0));
    updateRungHighlights();
}

LadderElementItem *LadderScene::createItemForElement(LadderElement *element, int rungIndex)
{
    if (element == nullptr) {
        return nullptr;
    }

    LadderElementItem *item = nullptr;
    switch (element->kind) {
    case LadderElementKind::Contact:
        item = new ContactItem(m_program, element);
        break;
    case LadderElementKind::Coil:
        item = new CoilItem(m_program, element);
        break;
    case LadderElementKind::FunctionBlock:
        item = new FunctionBlockItem(m_program, element);
        break;
    }

    if (item == nullptr) {
        return nullptr;
    }

    addItem(item);
    m_items.insert(element->localId, item);

    connect(item, &LadderElementItem::elementChanged, this, [this]() {
        emit programModified();
    });
    connect(item, &LadderElementItem::elementActivated, this, &LadderScene::elementActivated);
    connect(item, &LadderElementItem::elementDoubleClicked, this, &LadderScene::elementDoubleClicked);
    connect(item, &LadderElementItem::elementDragFinished, this, &LadderScene::commitElementDrag);
    connect(item, &LadderElementItem::elementActivated, this, [this, rungIndex](LadderElementItem *) {
        selectRung(rungIndex);
        clearBranchSelection();
    });
    return item;
}

LadderScene::RungMetrics LadderScene::rungMetrics(const LadderRung &rung) const
{
    RungMetrics metrics;
    for (const LadderElement &element : rung.elements) {
        metrics.maxRow = qMax(metrics.maxRow, element.row);
        metrics.maxCol = qMax(metrics.maxCol, element.col);
    }
    if (rung.branchRowsBelow > 0) {
        metrics.maxRow = qMax(metrics.maxRow, rung.branchRowsBelow);
    }
    return metrics;
}

qreal LadderScene::effectiveLayoutWidth(const RungMetrics &metrics) const
{
    const qreal contentWidth = LadderLayout::columnX(metrics.maxCol) + LadderElementItem::kCellWidth
                               + LadderLayout::kRightRailPad;
    return qMax(qMax(m_layoutWidth, contentWidth), m_contentWidth);
}

qreal LadderScene::rightRailXForMetrics(const RungMetrics &metrics) const
{
    return effectiveLayoutWidth(metrics) - LadderLayout::kRightRailPad;
}

int LadderScene::rungIndexForElement(int localId) const
{
    if (m_program == nullptr) {
        return -1;
    }
    for (int i = 0; i < m_program->rungs.size(); ++i) {
        for (const LadderElement &element : m_program->rungs[i].elements) {
            if (element.localId == localId) {
                return i;
            }
        }
    }
    return -1;
}

void LadderScene::selectRung(int index)
{
    if (m_program == nullptr || index < 0 || index >= m_program->rungs.size()) {
        return;
    }
    m_selectedRungIndex = index;
    clearBranchSelection();
    updateRungHighlights();
}

LadderElementItem *LadderScene::elementItemByLocalId(int localId) const
{
    return m_items.value(localId, nullptr);
}

void LadderScene::refreshElementItem(int localId)
{
    if (LadderElementItem *item = elementItemByLocalId(localId)) {
        item->update();
    }
}

void LadderScene::refreshAllElementItems()
{
    for (LadderElementItem *item : m_items) {
        if (item != nullptr) {
            item->update();
        }
    }
}

void LadderScene::updateRungHighlights()
{
    for (RungBandItem *band : m_rungBands) {
        if (band != nullptr) {
            band->setHighlighted(band->rungIndex() == m_selectedRungIndex);
        }
    }
}

int LadderScene::outputCoilColumn() const
{
    return LadderLayout::outputCoilColumn(qMax(m_layoutWidth, LadderLayout::kMinLayoutWidth));
}

void LadderScene::normalizeProgramLayout(int rungIndex)
{
    if (m_program == nullptr) {
        return;
    }

    const int coilCol = outputCoilColumn();
    if (rungIndex >= 0 && rungIndex < m_program->rungs.size()) {
        m_program->normalizeRungLayout(rungIndex, coilCol);
        m_program->autoWireRung(rungIndex);
        return;
    }

    m_program->normalizeAllRungs(coilCol);
    for (int i = 0; i < m_program->rungs.size(); ++i) {
        m_program->autoWireRung(i);
    }
}

bool LadderScene::mapPointToCell(const QPointF &pos, int *rungIndex, int *row, int *col) const
{
    if (rungIndex == nullptr || row == nullptr || col == nullptr || m_rungLayouts.isEmpty()) {
        return false;
    }

    for (const RungLayoutInfo &layout : m_rungLayouts) {
        if (pos.y() >= layout.yOffset - 4 && pos.y() <= layout.yOffset + layout.height + 4) {
            *rungIndex = layout.rungIndex;
            *row = LadderLayout::rowAtY(layout.yOffset, pos.y());
            *col = LadderLayout::columnAtX(pos.x());
            return true;
        }
    }

    const RungLayoutInfo &last = m_rungLayouts.last();
    *rungIndex = last.rungIndex;
    *row = LadderLayout::rowAtY(last.yOffset, pos.y());
    *col = LadderLayout::columnAtX(pos.x());
    return true;
}

void LadderScene::layoutRung(int rungIndex, qreal yOffset, qreal rightRailX, bool reuseItems)
{
    if (m_program == nullptr) {
        return;
    }

    LadderRung *rung = m_program->rungAt(rungIndex);
    if (rung == nullptr) {
        return;
    }

    const RungMetrics metrics = rungMetrics(*rung);
    const qreal rungHeight = (metrics.maxRow + 1) * LadderElementItem::kCellHeight;
    const qreal bandWidth = rightRailX + LadderLayout::kRightRailPad;

    RungLayoutInfo layoutInfo;
    layoutInfo.rungIndex = rungIndex;
    layoutInfo.yOffset = yOffset;
    layoutInfo.height = rungHeight;
    m_rungLayouts.push_back(layoutInfo);

    auto *rungBand = new RungBandItem(rungIndex, this);
    rungBand->setRect(0, yOffset - 4, bandWidth, rungHeight + 8);
    rungBand->setHighlighted(rungIndex == m_selectedRungIndex);
    addItem(rungBand);
    m_rungBands.push_back(rungBand);

    auto *numberColumn = new QGraphicsRectItem(0, yOffset - 4, LadderLayout::kLeftRailX, rungHeight + 8);
    numberColumn->setBrush(QColor("#e4e8ee"));
    numberColumn->setPen(Qt::NoPen);
    numberColumn->setZValue(-9);
    addItem(numberColumn);

    auto *rungLabel = new QGraphicsSimpleTextItem(QStringLiteral("%1").arg(rungIndex, 4, 10, QChar('0')));
    QFont rungFont(QStringLiteral("Consolas"), 10, QFont::Bold);
    rungLabel->setFont(rungFont);
    rungLabel->setBrush(QColor("#2d325a"));
    rungLabel->setPos(10, LadderLayout::rowBusY(yOffset, 0) - 12);
    rungLabel->setZValue(1);
    addItem(rungLabel);

    auto *rightRail = new QGraphicsLineItem(rightRailX,
                                            yOffset,
                                            rightRailX,
                                            yOffset + rungHeight);
    rightRail->setPen(QPen(QColor("#101010"), 3.0));
    rightRail->setZValue(-2);
    addItem(rightRail);

    QMap<int, LadderElementItem *> rungItems;
    for (LadderElement &element : m_program->rungs[rungIndex].elements) {
        LadderElementItem *item = reuseItems ? m_items.value(element.localId, nullptr)
                                             : createItemForElement(&element, rungIndex);
        if (item == nullptr) {
            continue;
        }
        item->setPos(LadderLayout::columnX(element.col), yOffset + element.row * LadderElementItem::kCellHeight);
        rungItems.insert(element.localId, item);
    }

    rung->ensureInputBranchRegions();
    drawRungWires(*rung, rungItems, yOffset, metrics.maxCol, rightRailX);
    layoutBranchRegions(rungIndex, yOffset);
    updateBranchSelectionVisuals();
}

void LadderScene::drawRungWires(const LadderRung &rung,
                                const QMap<int, LadderElementItem *> &items,
                                qreal yOffset,
                                int maxCol,
                                qreal rightRailX)
{
    const qreal leftBusX = LadderLayout::kLeftRailX;

    auto addJunctionDot = [this](qreal x, qreal y) {
        auto *dot = new QGraphicsEllipseItem(x - 3.5, y - 3.5, 7, 7);
        dot->setBrush(QColor("#101010"));
        dot->setPen(Qt::NoPen);
        dot->setZValue(0);
        addItem(dot);
    };

    auto wireHorizontal = [this](qreal x1, qreal x2, qreal y) {
        if (qAbs(x2 - x1) < 0.5) {
            return;
        }
        if (WireItem *wire = WireItem::addSegment(this, QLineF(x1, y, x2, y))) {
            m_wires.push_back(wire);
        }
    };

    auto wireVertical = [this](qreal x, qreal y1, qreal y2) {
        if (qAbs(y2 - y1) < 0.5) {
            return;
        }
        if (WireItem *wire = WireItem::addSegment(this, QLineF(x, y1, x, y2))) {
            m_wires.push_back(wire);
        }
    };

    auto wireRowElements = [&](int row, const QVector<const LadderElement *> &rowElements, qreal busY,
                               qreal startX, qreal endX) {
        qreal cursorX = startX;
        for (const LadderElement *element : rowElements) {
            const LadderElementItem *item = items.value(element->localId, nullptr);
            const qreal elementX = item != nullptr ? item->pos().x() : LadderLayout::columnX(element->col);
            wireHorizontal(cursorX, elementX, busY);
            cursorX = elementX + LadderElementItem::kCellWidth;
        }
        wireHorizontal(cursorX, endX, busY);
    };

    auto collectRowElements = [&](int row, auto predicate) {
        QVector<const LadderElement *> rowElements;
        for (const LadderElement &element : rung.elements) {
            if (element.row == row && predicate(element)) {
                rowElements.push_back(&element);
            }
        }
        std::sort(rowElements.begin(), rowElements.end(), [](const LadderElement *a, const LadderElement *b) {
            return a->col < b->col;
        });
        return rowElements;
    };

    int inputForkCol = 0;
    int inputJoinCol = 0;
    int outputForkCol = 0;
    const bool hasInput = rung.hasInputBranch() && rung.branchSpan(&inputForkCol, &inputJoinCol);
    const bool hasOutput = rung.hasOutputBranch() && rung.outputBranchForkColumn(&outputForkCol);
    const qreal mainBusY = LadderLayout::rowBusY(yOffset, 0);

    if (hasInput || hasOutput) {
        const int prefixEndCol = hasInput ? inputForkCol : outputForkCol;
        const qreal prefixEndX = LadderLayout::columnX(prefixEndCol);

        const QVector<const LadderElement *> prefix =
            collectRowElements(0, [&](const LadderElement &element) {
                return element.kind != LadderElementKind::Coil && element.col < prefixEndCol;
            });

        addJunctionDot(leftBusX, mainBusY);
        wireRowElements(0, prefix, mainBusY, leftBusX, prefixEndX);

        qreal feedX = prefixEndX;

        if (hasInput) {
            const qreal inputForkX = LadderLayout::columnX(inputForkCol);
            const qreal inputJoinX = LadderLayout::columnX(inputJoinCol);

            if (!hasOutput || prefixEndCol != inputForkCol) {
                wireHorizontal(feedX, inputForkX, mainBusY);
            }
            addJunctionDot(inputForkX, mainBusY);
            feedX = inputForkX;

            std::function<void(const LadderInputBranchRegion &, qreal)> drawInputRegion;
            drawInputRegion = [&](const LadderInputBranchRegion &region, qreal pathBusY) {
                const qreal forkX = LadderLayout::columnX(region.forkCol);
                const qreal joinX = LadderLayout::columnX(region.joinCol);

                const LadderInputBranchRegion *nested =
                    LadderBranchLogic::nestedInputRegionOnPathRow(rung, region);
                if (nested != nullptr) {
                    const qreal nestedForkX = LadderLayout::columnX(nested->forkCol);
                    const QVector<const LadderElement *> prefix =
                        collectRowElements(region.pathRow, [&](const LadderElement &element) {
                            return element.kind != LadderElementKind::Coil && element.col >= region.forkCol &&
                                   element.col < nested->forkCol;
                        });
                    if (prefix.isEmpty()) {
                        wireHorizontal(forkX, nestedForkX, pathBusY);
                    } else {
                        wireRowElements(region.pathRow, prefix, pathBusY, forkX, nestedForkX);
                    }
                    addJunctionDot(nestedForkX, pathBusY);

                    const qreal nestedBusY = LadderLayout::rowBusY(yOffset, nested->pathRow);
                    if (nested->pathRow > region.pathRow) {
                        wireVertical(nestedForkX, pathBusY, nestedBusY);
                        addJunctionDot(nestedForkX, nestedBusY);
                    }
                    drawInputRegion(*nested, nestedBusY);

                    const QVector<const LadderElement *> suffix =
                        collectRowElements(region.pathRow, [&](const LadderElement &element) {
                            return element.kind != LadderElementKind::Coil && element.col >= nested->joinCol &&
                                   element.col < region.joinCol;
                        });
                    const qreal nestedJoinX = LadderLayout::columnX(nested->joinCol);
                    if (suffix.isEmpty()) {
                        wireHorizontal(nestedJoinX, joinX, pathBusY);
                    } else {
                        wireRowElements(region.pathRow, suffix, pathBusY, nestedJoinX, joinX);
                    }
                } else {
                    const QVector<const LadderElement *> spanMain =
                        collectRowElements(region.pathRow, [&](const LadderElement &element) {
                            return element.kind != LadderElementKind::Coil && element.col >= region.forkCol &&
                                   element.col < region.joinCol;
                        });
                    if (spanMain.isEmpty()) {
                        wireHorizontal(forkX, joinX, pathBusY);
                    } else {
                        wireRowElements(region.pathRow, spanMain, pathBusY, forkX, joinX);
                    }
                }

                const int maxRow = LadderBranchLogic::maxParallelRowForRegion(rung, region);
                if (maxRow > region.pathRow) {
                    const qreal bottomBusY = LadderLayout::rowBusY(yOffset, maxRow);
                    if (bottomBusY > pathBusY) {
                        wireVertical(forkX, pathBusY, bottomBusY);
                    }
                }

                for (int row = region.pathRow + 1; row <= maxRow; ++row) {
                    const qreal branchBusY = LadderLayout::rowBusY(yOffset, row);
                    addJunctionDot(forkX, branchBusY);

                    const LadderInputBranchRegion *child =
                        LadderBranchLogic::childInputRegionOnRow(rung, region, row);
                    if (child != nullptr) {
                        drawInputRegion(*child, branchBusY);
                    } else {
                        const QVector<const LadderElement *> branchElements =
                            collectRowElements(row, [&](const LadderElement &element) {
                                return element.kind != LadderElementKind::Coil && element.col >= region.forkCol &&
                                       element.col < region.joinCol;
                            });
                        if (branchElements.isEmpty()) {
                            wireHorizontal(forkX, joinX, branchBusY);
                        } else {
                            wireRowElements(row, branchElements, branchBusY, forkX, joinX);
                        }
                    }

                    addJunctionDot(joinX, branchBusY);
                    if (branchBusY > pathBusY) {
                        wireVertical(joinX, branchBusY, pathBusY);
                    }
                }
            };

            if (!rung.inputBranchRegions.isEmpty()) {
                const QVector<const LadderInputBranchRegion *> roots = LadderBranchLogic::rootInputRegions(rung);
                if (!roots.isEmpty()) {
                    drawInputRegion(*roots.first(), mainBusY);
                }
            } else {
                const QVector<const LadderElement *> spanMain =
                    collectRowElements(0, [&](const LadderElement &element) {
                        return element.kind != LadderElementKind::Coil && element.col >= inputForkCol &&
                               element.col < inputJoinCol;
                    });
                if (spanMain.isEmpty()) {
                    wireHorizontal(inputForkX, inputJoinX, mainBusY);
                } else {
                    wireRowElements(0, spanMain, mainBusY, inputForkX, inputJoinX);
                }

                int maxInputBranchRow = 0;
                for (const LadderElement &element : rung.elements) {
                    if (element.row > 0 && element.kind != LadderElementKind::Coil &&
                        element.col >= inputForkCol && element.col < inputJoinCol) {
                        maxInputBranchRow = qMax(maxInputBranchRow, element.row);
                    }
                }

                if (maxInputBranchRow > 0) {
                    wireVertical(inputForkX, mainBusY, LadderLayout::rowBusY(yOffset, maxInputBranchRow));
                }

                for (int row = 1; row <= maxInputBranchRow; ++row) {
                    const QVector<const LadderElement *> branchElements =
                        collectRowElements(row, [&](const LadderElement &element) {
                            return element.kind != LadderElementKind::Coil && element.col >= inputForkCol &&
                                   element.col < inputJoinCol;
                        });

                    const qreal branchBusY = LadderLayout::rowBusY(yOffset, row);
                    addJunctionDot(inputForkX, branchBusY);
                    if (branchElements.isEmpty()) {
                        wireHorizontal(inputForkX, inputJoinX, branchBusY);
                    } else {
                        wireRowElements(row, branchElements, branchBusY, inputForkX, inputJoinX);
                    }
                    addJunctionDot(inputJoinX, branchBusY);
                    wireVertical(inputJoinX, branchBusY, mainBusY);
                }
            }

            addJunctionDot(inputJoinX, mainBusY);
            feedX = inputJoinX;

            const int suffixEndCol = hasOutput ? outputForkCol : (maxCol + 1);
            const QVector<const LadderElement *> suffix =
                collectRowElements(0, [&](const LadderElement &element) {
                    return element.kind != LadderElementKind::Coil && element.col >= inputJoinCol &&
                           element.col < suffixEndCol;
                });

            if (hasOutput) {
                const qreal outputForkX = LadderLayout::columnX(outputForkCol);
                if (suffix.isEmpty()) {
                    wireHorizontal(inputJoinX, outputForkX, mainBusY);
                } else {
                    wireRowElements(0, suffix, mainBusY, inputJoinX, outputForkX);
                }
                addJunctionDot(outputForkX, mainBusY);
                feedX = outputForkX;
            } else {
                const QVector<const LadderElement *> rowZeroCoils =
                    collectRowElements(0, [](const LadderElement &element) {
                        return element.kind == LadderElementKind::Coil;
                    });
                const LadderElement *coil = rowZeroCoils.isEmpty() ? nullptr : rowZeroCoils.first();
                if (coil != nullptr) {
                    const LadderElementItem *coilItem = items.value(coil->localId, nullptr);
                    const qreal coilX = coilItem != nullptr ? coilItem->pos().x()
                                                            : LadderLayout::columnX(coil->col);
                    if (suffix.isEmpty()) {
                        wireHorizontal(inputJoinX, coilX, mainBusY);
                    } else {
                        wireRowElements(0, suffix, mainBusY, inputJoinX, coilX);
                    }
                    const qreal coilEndX = coilX + LadderElementItem::kCellWidth;
                    wireHorizontal(coilEndX, rightRailX, mainBusY);
                    addJunctionDot(rightRailX, mainBusY);
                } else if (!suffix.isEmpty()) {
                    wireRowElements(0, suffix, mainBusY, inputJoinX, rightRailX);
                    addJunctionDot(rightRailX, mainBusY);
                }
                return;
            }
        }

        if (hasOutput) {
            QVector<const LadderElement *> coils;
            for (const LadderElement &element : rung.elements) {
                if (element.kind == LadderElementKind::Coil) {
                    coils.push_back(&element);
                }
            }
            std::sort(coils.begin(), coils.end(), [](const LadderElement *a, const LadderElement *b) {
                if (a->row != b->row) {
                    return a->row < b->row;
                }
                return a->col < b->col;
            });

            const qreal outputForkX = LadderLayout::columnX(outputForkCol);
            if (!hasInput) {
                wireHorizontal(feedX, outputForkX, mainBusY);
                addJunctionDot(outputForkX, mainBusY);
            }

            int maxCoilRow = 0;
            for (const LadderElement *coil : coils) {
                maxCoilRow = qMax(maxCoilRow, coil->row);
            }

            if (maxCoilRow > 0) {
                const qreal bottomCoilBusY = LadderLayout::rowBusY(yOffset, maxCoilRow);
                wireVertical(outputForkX, mainBusY, bottomCoilBusY);
            }

            for (const LadderElement *coil : coils) {
                const QVector<const LadderElement *> rowPath =
                    collectRowElements(coil->row, [&](const LadderElement &element) {
                        return element.kind != LadderElementKind::Coil && element.col >= outputForkCol &&
                               element.col < coil->col;
                    });
                const qreal branchBusY = LadderLayout::rowBusY(yOffset, coil->row);
                const LadderElementItem *coilItem = items.value(coil->localId, nullptr);
                const qreal coilX = coilItem != nullptr ? coilItem->pos().x() : LadderLayout::columnX(coil->col);

                addJunctionDot(outputForkX, branchBusY);
                if (rowPath.isEmpty()) {
                    wireHorizontal(outputForkX, coilX, branchBusY);
                } else {
                    wireRowElements(coil->row, rowPath, branchBusY, outputForkX, coilX);
                }

                const qreal coilEndX = coilX + LadderElementItem::kCellWidth;
                wireHorizontal(coilEndX, rightRailX, branchBusY);
                addJunctionDot(rightRailX, branchBusY);
            }
        }
        return;
    }

    QSet<int> rows;
    for (const LadderElement &element : rung.elements) {
        rows.insert(element.row);
    }

    for (int row : rows) {
        const QVector<const LadderElement *> rowElements = elementsOnRow(rung, row);
        if (rowElements.isEmpty()) {
            continue;
        }

        const qreal busY = LadderLayout::rowBusY(yOffset, row);
        const LadderElementItem *firstItem = items.value(rowElements.first()->localId, nullptr);
        const LadderElementItem *lastItem = items.value(rowElements.last()->localId, nullptr);

        const qreal rowStartX = firstItem != nullptr ? firstItem->pos().x()
                                                     : LadderLayout::columnX(rowElements.first()->col);
        const qreal rowEndX = lastItem != nullptr ? lastItem->pos().x() + LadderElementItem::kCellWidth
                                                  : LadderLayout::columnX(rowElements.last()->col)
                                                        + LadderElementItem::kCellWidth;

        wireHorizontal(leftBusX, rowStartX, busY);
        addJunctionDot(leftBusX, busY);

        for (int i = 0; i + 1 < rowElements.size(); ++i) {
            const LadderElementItem *fromItem = items.value(rowElements[i]->localId, nullptr);
            const LadderElementItem *toItem = items.value(rowElements[i + 1]->localId, nullptr);
            if (fromItem == nullptr || toItem == nullptr) {
                continue;
            }
            wireHorizontal(fromItem->pos().x() + LadderElementItem::kCellWidth,
                           toItem->pos().x(),
                           busY);
        }

        if (row == 0) {
            wireHorizontal(rowEndX, rightRailX, busY);
            addJunctionDot(rightRailX, busY);
        }
    }
}

void LadderScene::rebuild()
{
    clearScene();
    if (m_program == nullptr) {
        emit layoutCompleted();
        return;
    }

    normalizeProgramLayout();

    RungMetrics overallMetrics;
    for (const LadderRung &rung : m_program->rungs) {
        const RungMetrics metrics = rungMetrics(rung);
        overallMetrics.maxRow = qMax(overallMetrics.maxRow, metrics.maxRow);
        overallMetrics.maxCol = qMax(overallMetrics.maxCol, metrics.maxCol);
    }

    m_contentWidth = LadderLayout::columnX(overallMetrics.maxCol) + LadderElementItem::kCellWidth
                     + LadderLayout::kRightRailPad;
    const qreal rightRailX = rightRailXForMetrics(overallMetrics);

    qreal y = LadderLayout::kTopMargin;
    qreal contentBottom = y;

    for (int i = 0; i < m_program->rungs.size(); ++i) {
        layoutRung(i, y, rightRailX);
        const RungMetrics metrics = rungMetrics(m_program->rungs[i]);
        contentBottom = y + (metrics.maxRow + 1) * LadderElementItem::kCellHeight;
        y = contentBottom + LadderLayout::kRungSpacing;
    }

    if (contentBottom > LadderLayout::kTopMargin) {
        auto *leftRail = new QGraphicsLineItem(LadderLayout::kLeftRailX,
                                               LadderLayout::kTopMargin,
                                               LadderLayout::kLeftRailX,
                                               contentBottom);
        leftRail->setPen(QPen(QColor("#101010"), 3.0));
        leftRail->setZValue(-3);
        addItem(leftRail);
    }

    const qreal sceneWidth = qMax(m_layoutWidth, m_contentWidth);
    setSceneRect(0, 0, sceneWidth, qMax<qreal>(600.0, contentBottom + 80.0));
    updateRungHighlights();
    emit layoutCompleted();
}

void LadderScene::refreshSelectedRung()
{
    if (m_program == nullptr) {
        return;
    }
    if (m_selectedRungIndex < 0 || m_selectedRungIndex >= m_program->rungs.size()) {
        m_selectedRungIndex = qMax(0, m_program->rungs.size() - 1);
    }
}

void LadderScene::insertElementBeforeCoil(LadderRung &rung, int row, const std::function<void(int col)> &insert)
{
    int insertCol = 1;
    for (const LadderElement &element : rung.elements) {
        if (element.row == row && element.kind != LadderElementKind::Coil) {
            insertCol = qMax(insertCol, element.col + 1);
        }
    }

    for (LadderElement &element : rung.elements) {
        if (element.row == row && element.kind != LadderElementKind::Coil && element.col >= insertCol) {
            element.col++;
        }
    }

    insert(insertCol);
}

void LadderScene::insertElementAt(int rungIndex, int row, int col, const std::function<void(int col)> &insert)
{
    if (m_program == nullptr) {
        return;
    }

    LadderRung *rung = m_program->rungAt(rungIndex);
    if (rung == nullptr) {
        return;
    }

    int targetCol = qMax(1, col);
    bool shiftCoilOnRow = false;
    for (const LadderElement &element : rung->elements) {
        if (element.row == row && element.kind == LadderElementKind::Coil && element.col == targetCol) {
            shiftCoilOnRow = true;
            break;
        }
    }

    for (LadderElement &element : rung->elements) {
        if (element.row != row || element.col < targetCol) {
            continue;
        }
        if (element.kind == LadderElementKind::Coil && !shiftCoilOnRow) {
            continue;
        }
        element.col++;
    }

    insert(targetCol);
}

namespace {

bool constrainBranchCell(LadderRung *rung, LadderElementKind kind, int *row, int *col)
{
    if (rung == nullptr || row == nullptr || col == nullptr) {
        return false;
    }
    if (kind == LadderElementKind::Coil && *row > 0) {
        return false;
    }

    rung->ensureInputBranchRegions();

    auto tryOutputBranch = [&]() -> bool {
        if (kind == LadderElementKind::Coil) {
            return false;
        }
        int forkCol = 0;
        int maxInsertCol = 0;
        if (!LadderBranchLogic::outputBranchInstructionSpan(*rung, *row, &forkCol, &maxInsertCol)) {
            return false;
        }
        *col = qBound(forkCol, *col, maxInsertCol);
        rung->branchRowsBelow = qMax(rung->branchRowsBelow, *row);
        return true;
    };

    if (*row > 0) {
        const LadderInputBranchRegion *region =
            LadderBranchLogic::regionForBranchPlacement(*rung, *row, *col);
        if (region != nullptr) {
            const int maxCol = LadderBranchLogic::maxInsertColForBranchRow(*rung, *region, *row);
            *col = qBound(region->forkCol, *col, maxCol);
            rung->branchRowsBelow = qMax(rung->branchRowsBelow, *row);
            return true;
        }

        return tryOutputBranch();
    }

    const LadderInputBranchRegion *region = LadderBranchLogic::regionForBranchPlacement(*rung, *row, *col);
    if (region != nullptr) {
        const int maxCol = LadderBranchLogic::maxInsertColForBranchRow(*rung, *region, *row);
        *col = qBound(region->forkCol, *col, maxCol);
        return true;
    }

    if (tryOutputBranch()) {
        return true;
    }

    return true;
}

} // namespace

void LadderScene::insertElementAfterSelectionOrBeforeCoil(LadderElementKind kind,
                                                          const std::function<void(int row, int col)> &insert)
{
    if (m_program == nullptr || !insert) {
        return;
    }

    refreshSelectedRung();

    int targetRow = 0;
    int targetCol = 1;
    bool useSelection = false;
    bool insertBeforeCoil = false;

    for (QGraphicsItem *graphicsItem : selectedItems()) {
        auto *item = dynamic_cast<LadderElementItem *>(graphicsItem);
        if (item == nullptr) {
            continue;
        }
        const LadderElement *element = item->element();
        if (element == nullptr) {
            continue;
        }
        if (element->kind == LadderElementKind::Coil) {
            targetRow = element->row;
            targetCol = element->col;
            useSelection = true;
            insertBeforeCoil = true;
            break;
        }
        targetRow = element->row;
        targetCol = element->col + 1;
        useSelection = true;
        break;
    }

    if (useSelection) {
        LadderRung *rung = m_program->rungAt(m_selectedRungIndex);
        if (rung == nullptr) {
            return;
        }
        if (!constrainBranchCell(rung, kind, &targetRow, &targetCol)) {
            emit statusMessage(
                insertBeforeCoil
                    ? QStringLiteral("Cannot add here — drop onto the output branch row before the coil.")
                    : QStringLiteral("Cannot add here — select an element inside a branch leg, or drop onto the branch row."));
            return;
        }
        insertElementAt(m_selectedRungIndex, targetRow, targetCol,
                          [&](int col) { insert(targetRow, col); });
        return;
    }

    LadderRung *rung = m_program->rungAt(m_selectedRungIndex);
    if (rung == nullptr) {
        return;
    }
    insertElementBeforeCoil(*rung, 0, [&](int col) { insert(0, col); });
}

void LadderScene::moveElementToCell(int localId, int rungIndex, int row, int col)
{
    if (m_program == nullptr) {
        return;
    }

    const int sourceRung = rungIndexForElement(localId);
    if (sourceRung < 0) {
        return;
    }

    LadderElement moved;
    bool found = false;
    for (int i = 0; i < m_program->rungs[sourceRung].elements.size(); ++i) {
        if (m_program->rungs[sourceRung].elements[i].localId == localId) {
            moved = m_program->rungs[sourceRung].elements[i];
            m_program->rungs[sourceRung].elements.removeAt(i);
            found = true;
            break;
        }
    }
    if (!found || rungIndex < 0 || rungIndex >= m_program->rungs.size()) {
        rebuild();
        return;
    }

    moved.rungIndex = rungIndex;
    moved.row = qMax(0, row);
    moved.col = qMax(1, col);

    LadderRung *targetRung = m_program->rungAt(rungIndex);
    if (targetRung == nullptr) {
        rebuild();
        return;
    }

    if (!constrainBranchCell(targetRung, moved.kind, &moved.row, &moved.col)) {
        rebuild();
        return;
    }

    if (moved.kind == LadderElementKind::Coil) {
        targetRung->elements.push_back(moved);
    } else {
        insertElementAt(rungIndex, moved.row, moved.col, [&](int insertCol) {
            moved.col = insertCol;
            targetRung->elements.push_back(moved);
        });
    }

    normalizeProgramLayout(sourceRung);
    if (sourceRung != rungIndex) {
        normalizeProgramLayout(rungIndex);
    }
    m_selectedRungIndex = rungIndex;
    rebuild();
    emit programModified();
}

bool LadderScene::parseDropPayload(const QMimeData *mimeData, LadderElementKind *kind, bool *negated,
                                   LadderCoilKind *coilKind, QString *fbType) const
{
    if (mimeData == nullptr || kind == nullptr || negated == nullptr || coilKind == nullptr || fbType == nullptr) {
        return false;
    }

    const QString payload = QString::fromUtf8(mimeData->data(LadderMime::mimeType()));
    if (payload.startsWith(QStringLiteral("contact/"))) {
        *kind = LadderElementKind::Contact;
        *negated = payload.endsWith(QStringLiteral("nc"));
        return true;
    }
    if (payload.startsWith(QStringLiteral("coil/"))) {
        *kind = LadderElementKind::Coil;
        if (payload.endsWith(QStringLiteral("set"))) {
            *coilKind = LadderCoilKind::Set;
        } else if (payload.endsWith(QStringLiteral("reset"))) {
            *coilKind = LadderCoilKind::Reset;
        } else {
            *coilKind = LadderCoilKind::Normal;
        }
        return true;
    }
    if (payload.startsWith(QStringLiteral("fb/"))) {
        *kind = LadderElementKind::FunctionBlock;
        *fbType = payload.mid(3);
        return !fbType->isEmpty();
    }
    return false;
}

void LadderScene::placeDroppedElement(int rungIndex, int row, int col, LadderElementKind kind, bool negated,
                                        LadderCoilKind coilKind, const QString &fbType)
{
    if (m_program == nullptr) {
        return;
    }

    selectRung(rungIndex);

    LadderRung *rung = m_program->rungAt(rungIndex);
    if (rung == nullptr) {
        return;
    }
    if (!constrainBranchCell(rung, kind, &row, &col)) {
        if (row > 0) {
            emit statusMessage(
                QStringLiteral("Drop onto an input branch row, or onto an output branch row before its coil."));
        }
        return;
    }

    if (kind == LadderElementKind::Coil) {
        insertElementAt(rungIndex, row, col, [&](int insertCol) {
            m_program->addCoil(rungIndex, row, insertCol,
                               LadderIoCatalog::outputNameForRung(rungIndex), coilKind);
        });
    } else if (kind == LadderElementKind::FunctionBlock) {
        insertElementAt(rungIndex, row, col, [&](int insertCol) {
            m_program->addFunctionBlock(rungIndex, row, insertCol, fbType);
        });
    } else {
        insertElementAt(rungIndex, row, col, [&](int insertCol) {
            m_program->addContact(rungIndex, row, insertCol,
                                  LadderIoCatalog::inputNameForRung(rungIndex), negated);
        });
    }

    normalizeProgramLayout(rungIndex);
    rebuild();
    emit programModified();
}

bool LadderScene::dropElementPayload(const QPointF &scenePos, const QMimeData *mimeData)
{
    LadderElementKind kind = LadderElementKind::Contact;
    bool negated = false;
    LadderCoilKind coilKind = LadderCoilKind::Normal;
    QString fbType;
    if (!parseDropPayload(mimeData, &kind, &negated, &coilKind, &fbType)) {
        return false;
    }

    int rungIndex = 0;
    int row = 0;
    int col = 1;
    if (!mapPointToCell(scenePos, &rungIndex, &row, &col)) {
        rungIndex = m_selectedRungIndex;
    }

    placeDroppedElement(rungIndex, row, col, kind, negated, coilKind, fbType);
    return true;
}

void LadderScene::commitElementDrag(LadderElementItem *item)
{
    if (item == nullptr || m_program == nullptr) {
        rebuild();
        return;
    }

    const LadderElement *element = item->element();
    if (element == nullptr) {
        rebuild();
        return;
    }

    const QPointF anchor = item->pos()
                           + QPointF(LadderElementItem::kCellWidth * 0.5, LadderElementItem::kBusLineY);
    int rungIndex = 0;
    int row = 0;
    int col = 1;
    if (!mapPointToCell(anchor, &rungIndex, &row, &col)) {
        rebuild();
        return;
    }

    LadderRung *rung = m_program->rungAt(rungIndex);
    if (rung == nullptr || !constrainBranchCell(rung, element->kind, &row, &col)) {
        rebuild();
        return;
    }

    moveElementToCell(item->localId(), rungIndex, row, col);
}

void LadderScene::addRung()
{
    if (m_program == nullptr) {
        return;
    }

    const int rungIndex = m_program->rungs.size();
    m_program->addRung();
    normalizeProgramLayout(rungIndex);
    m_selectedRungIndex = rungIndex;
    rebuild();
    emit programModified();
}

void LadderScene::removeSelectedRung()
{
    if (m_program == nullptr || m_program->rungs.isEmpty()) {
        return;
    }

    refreshSelectedRung();
    m_program->removeRung(m_selectedRungIndex);
    m_selectedRungIndex = qMin(m_selectedRungIndex, m_program->rungs.size() - 1);
    rebuild();
    emit programModified();
}

void LadderScene::handleDeleteKey()
{
    if (m_outputBranchSelected && m_selectedBranchRungIndex >= 0) {
        removeBranchFromSelectedRung();
        return;
    }
    if (m_selectedInputBranchRegionIndex >= 0 && m_selectedBranchRungIndex >= 0) {
        removeBranchFromSelectedRung();
        return;
    }

    for (QGraphicsItem *graphicsItem : selectedItems()) {
        if (dynamic_cast<LadderElementItem *>(graphicsItem) != nullptr) {
            removeSelectedElements();
            return;
        }
    }
    removeSelectedRung();
}

void LadderScene::removeSelectedElements()
{
    if (m_program == nullptr) {
        return;
    }

    const QList<QGraphicsItem *> selected = selectedItems();
    QSet<int> removeIds;
    for (QGraphicsItem *graphicsItem : selected) {
        auto *item = dynamic_cast<LadderElementItem *>(graphicsItem);
        if (item != nullptr) {
            removeIds.insert(item->localId());
        }
    }

    if (removeIds.isEmpty()) {
        return;
    }

    QSet<int> affectedRungs;
    bool removedBranchElement = false;
    for (const LadderRung &rung : m_program->rungs) {
        for (const LadderElement &element : rung.elements) {
            if (removeIds.contains(element.localId)) {
                affectedRungs.insert(element.rungIndex);
                if (element.row > 0) {
                    removedBranchElement = true;
                }
            }
        }
    }

    for (LadderRung &rung : m_program->rungs) {
        rung.elements.erase(std::remove_if(rung.elements.begin(),
                                             rung.elements.end(),
                                             [&](const LadderElement &element) {
                                                 return removeIds.contains(element.localId);
                                             }),
                            rung.elements.end());
    }

    if (removedBranchElement) {
        for (int rungIndex : affectedRungs) {
            LadderRung *rung = m_program->rungAt(rungIndex);
            if (rung == nullptr) {
                continue;
            }
            int forkCol = 0;
            int joinCol = 0;
            if (!rung->branchSpan(&forkCol, &joinCol)) {
                continue;
            }
            bool hasBranchElements = false;
            for (const LadderElement &element : rung->elements) {
                if (element.row > 0 && element.kind != LadderElementKind::Coil && element.col >= forkCol
                    && element.col < joinCol) {
                    hasBranchElements = true;
                    break;
                }
            }
            if (!hasBranchElements) {
                rung->branchForkCol = 0;
                rung->branchJoinCol = 0;
                rung->branchRowsBelow = 0;
            }
        }
    }

    refreshSelectedRung();
    normalizeProgramLayout(m_selectedRungIndex);
    rebuild();
    emit programModified();
}

bool LadderScene::mapScenePosToCell(const QPointF &pos, int *rungIndex, int *row, int *col) const
{
    return mapPointToCell(pos, rungIndex, row, col);
}

void LadderScene::addContactAtCell(int rungIndex, int row, int col, bool negated)
{
    if (m_program == nullptr) {
        return;
    }
    selectRung(rungIndex);
    placeDroppedElement(rungIndex, row, col, LadderElementKind::Contact, negated, LadderCoilKind::Normal,
                        QString());
}

void LadderScene::addParallelBranchLeg(bool negated)
{
    if (m_program == nullptr) {
        return;
    }
    refreshSelectedRung();

    int regionIndex = -1;
    if (m_selectedInputBranchRegionIndex >= 0 && m_selectedBranchRungIndex == m_selectedRungIndex) {
        regionIndex = m_selectedInputBranchRegionIndex;
    }

    QString errorMessage;
    if (!m_program->addParallelBranchLeg(m_selectedRungIndex, regionIndex, true, negated, &errorMessage)) {
        if (!errorMessage.isEmpty()) {
            emit statusMessage(errorMessage);
        }
        return;
    }

    normalizeProgramLayout(m_selectedRungIndex);
    rebuild();
    emit programModified();
    emit statusMessage(QStringLiteral("Added parallel branch leg. Edit the contact variable in Properties."));
}

void LadderScene::removeBranchFromSelectedRung()
{
    if (m_program == nullptr) {
        return;
    }
    refreshSelectedRung();

    int inputRegionIndex = -1;
    bool removeOutput = false;

    if (m_outputBranchSelected && m_selectedBranchRungIndex == m_selectedRungIndex) {
        removeOutput = true;
    } else if (m_selectedInputBranchRegionIndex >= 0
               && m_selectedBranchRungIndex == m_selectedRungIndex) {
        inputRegionIndex = m_selectedInputBranchRegionIndex;
    } else {
        QVector<int> selectedIds;
        for (QGraphicsItem *graphicsItem : selectedItems()) {
            if (auto *item = dynamic_cast<LadderElementItem *>(graphicsItem)) {
                selectedIds.push_back(item->localId());
            }
        }
        if (!selectedIds.isEmpty()) {
            const LadderRung *rung = m_program->rungAt(m_selectedRungIndex);
            if (rung != nullptr) {
                inputRegionIndex =
                    LadderBranchLogic::inferInputBranchRegionFromElements(*rung, selectedIds);
            }
        }
    }

    QString errorMessage;
    if (!m_program->removeBranch(m_selectedRungIndex, inputRegionIndex, removeOutput, &errorMessage)) {
        if (!errorMessage.isEmpty()) {
            emit statusMessage(errorMessage);
        }
        return;
    }

    clearBranchSelection();
    normalizeProgramLayout(m_selectedRungIndex);
    rebuild();
    emit programModified();
}

void LadderScene::addContactToSelectedRung(bool negated)
{
    if (m_program == nullptr) {
        return;
    }
    refreshSelectedRung();

    insertElementAfterSelectionOrBeforeCoil(LadderElementKind::Contact, [&](int row, int col) {
        m_program->addContact(m_selectedRungIndex, row, col,
                              LadderIoCatalog::inputNameForRung(m_selectedRungIndex), negated);
    });
    normalizeProgramLayout(m_selectedRungIndex);
    rebuild();
    emit programModified();
}

void LadderScene::addCoilToSelectedRung(LadderCoilKind kind)
{
    if (m_program == nullptr) {
        return;
    }
    refreshSelectedRung();

    LadderRung *rung = m_program->rungAt(m_selectedRungIndex);
    if (rung == nullptr) {
        return;
    }

    int existingCoils = 0;
    for (const LadderElement &element : rung->elements) {
        if (element.kind == LadderElementKind::Coil) {
            ++existingCoils;
        }
    }

    if (existingCoils > 0) {
        addOutputBranchCoilToSelectedRung(kind);
        return;
    }

    int nextCol = 1;
    for (const LadderElement &element : rung->elements) {
        if (element.row == 0) {
            nextCol = qMax(nextCol, element.col + 1);
        }
    }

    m_program->addCoil(m_selectedRungIndex, 0, nextCol,
                       LadderIoCatalog::outputNameForRung(m_selectedRungIndex), kind);
    normalizeProgramLayout(m_selectedRungIndex);
    rebuild();
    emit programModified();
}

void LadderScene::addOutputBranchCoilToSelectedRung(LadderCoilKind kind)
{
    if (m_program == nullptr) {
        return;
    }
    refreshSelectedRung();

    LadderRung *rung = m_program->rungAt(m_selectedRungIndex);
    if (rung == nullptr) {
        return;
    }

    m_program->addOutputBranchCoil(m_selectedRungIndex, kind);
    normalizeProgramLayout(m_selectedRungIndex);
    rebuild();
    emit programModified();
}

void LadderScene::addFunctionBlockToSelectedRung(const QString &typeName)
{
    if (m_program == nullptr || typeName.isEmpty()) {
        return;
    }
    refreshSelectedRung();

    insertElementAfterSelectionOrBeforeCoil(LadderElementKind::FunctionBlock, [&](int row, int col) {
        m_program->addFunctionBlock(m_selectedRungIndex, row, col, typeName);
    });
    normalizeProgramLayout(m_selectedRungIndex);
    rebuild();
    emit programModified();
}

void LadderScene::surroundSelectionWithBranch()
{
    if (m_program == nullptr) {
        return;
    }
    refreshSelectedRung();

    QVector<int> localIds;
    for (QGraphicsItem *graphicsItem : selectedItems()) {
        auto *item = dynamic_cast<LadderElementItem *>(graphicsItem);
        if (item != nullptr) {
            localIds.push_back(item->localId());
        }
    }

    QString errorMessage;
    if (!m_program->surroundWithBranch(m_selectedRungIndex, localIds, &errorMessage)) {
        if (!errorMessage.isEmpty()) {
            emit statusMessage(errorMessage);
        }
        return;
    }

    normalizeProgramLayout(m_selectedRungIndex);
    rebuild();
    emit programModified();

    const LadderRung &rung = m_program->rungs[m_selectedRungIndex];
    if (!localIds.isEmpty()) {
        bool selectedCoils = false;
        bool selectedInputs = false;
        for (int localId : localIds) {
            const LadderElement *element = m_program->elementById(localId);
            if (element == nullptr) {
                continue;
            }
            if (element->kind == LadderElementKind::Coil) {
                selectedCoils = true;
            } else {
                selectedInputs = true;
            }
        }
        if (selectedInputs) {
            emit statusMessage(
                QStringLiteral("Input branch created with a parallel leg. Use + Leg for more paths, or edit the new contact."));
        } else if (selectedCoils) {
            emit statusMessage(QStringLiteral("Output branch created with a second coil."));
        }
    }

    QVector<int> reselectIds = localIds;
    for (const LadderElement &element : m_program->rungs[m_selectedRungIndex].elements) {
        if (element.kind == LadderElementKind::Coil) {
            reselectIds.push_back(element.localId);
        }
    }
    QSet<int> uniqueIds(reselectIds.begin(), reselectIds.end());
    reselectIds = QVector<int>(uniqueIds.begin(), uniqueIds.end());

    for (int localId : reselectIds) {
        if (LadderElementItem *item = elementItemByLocalId(localId)) {
            item->setSelected(true);
        }
    }
}

void LadderScene::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) {
        handleDeleteKey();
        event->accept();
        return;
    }
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_D) {
            for (QGraphicsItem *graphicsItem : selectedItems()) {
                if (auto *contact = dynamic_cast<ContactItem *>(graphicsItem)) {
                    contact->toggleNegated();
                    event->accept();
                    return;
                }
                if (auto *coil = dynamic_cast<CoilItem *>(graphicsItem)) {
                    coil->cycleCoilKind();
                    event->accept();
                    return;
                }
            }
        }
    }

    QGraphicsScene::keyPressEvent(event);
}
