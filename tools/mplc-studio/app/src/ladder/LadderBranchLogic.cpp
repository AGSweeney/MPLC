/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderBranchLogic.h"
#include "LadderModel.h"

#include <QSet>
#include <algorithm>
#include <limits>

void LadderBranchLogic::migrateLegacyInputBranch(LadderRung *rung)
{
    if (rung == nullptr || !rung->inputBranchRegions.isEmpty()) {
        return;
    }
    if (rung->branchForkCol > 0 && rung->branchJoinCol > rung->branchForkCol) {
        LadderInputBranchRegion region;
        region.forkCol = rung->branchForkCol;
        region.joinCol = rung->branchJoinCol;
        region.pathRow = 0;
        rung->inputBranchRegions.push_back(region);
    }
}

void LadderBranchLogic::syncLegacyInputBranchFields(LadderRung *rung)
{
    if (rung == nullptr) {
        return;
    }
    migrateLegacyInputBranch(rung);
    if (rung->inputBranchRegions.isEmpty()) {
        rung->branchForkCol = 0;
        rung->branchJoinCol = 0;
        return;
    }
    const LadderInputBranchRegion *root = nullptr;
    for (const LadderInputBranchRegion &region : rung->inputBranchRegions) {
        if (region.pathRow == 0) {
            root = &region;
            break;
        }
    }
    if (root == nullptr) {
        root = &rung->inputBranchRegions.first();
    }
    rung->branchForkCol = root->forkCol;
    rung->branchJoinCol = root->joinCol;
}

QVector<LadderInputBranchRegion> LadderBranchLogic::inputBranchRegions(const LadderRung &rung)
{
    LadderRung copy = rung;
    migrateLegacyInputBranch(&copy);
    return copy.inputBranchRegions;
}

bool LadderBranchLogic::regionNestContains(const LadderInputBranchRegion &parent,
                                           const LadderInputBranchRegion &child)
{
    if (child.forkCol < parent.forkCol || child.joinCol > parent.joinCol) {
        return false;
    }
    if (child.pathRow < parent.pathRow) {
        return false;
    }
    if (child.pathRow == parent.pathRow) {
        return child.forkCol > parent.forkCol && child.joinCol < parent.joinCol;
    }
    return true;
}

const LadderInputBranchRegion *LadderBranchLogic::inputRegionForCell(const LadderRung &rung,
                                                                     int row,
                                                                     int col)
{
    LadderRung copy = rung;
    migrateLegacyInputBranch(&copy);
    const LadderInputBranchRegion *best = nullptr;
    int bestPathRow = -1;
    int bestSpan = std::numeric_limits<int>::max();

    for (const LadderInputBranchRegion &region : copy.inputBranchRegions) {
        if (col < region.forkCol || col >= region.joinCol) {
            continue;
        }
        if (row < region.pathRow) {
            continue;
        }
        const int span = region.joinCol - region.forkCol;
        if (region.pathRow > bestPathRow || (region.pathRow == bestPathRow && span < bestSpan)) {
            best = nullptr;
            for (const LadderInputBranchRegion &candidate : rung.inputBranchRegions) {
                if (candidate.forkCol == region.forkCol && candidate.joinCol == region.joinCol &&
                    candidate.pathRow == region.pathRow) {
                    best = &candidate;
                    break;
                }
            }
            if (best == nullptr && !copy.inputBranchRegions.isEmpty()) {
                for (const LadderInputBranchRegion &candidate : copy.inputBranchRegions) {
                    if (candidate.forkCol == region.forkCol && candidate.joinCol == region.joinCol &&
                        candidate.pathRow == region.pathRow) {
                        best = &candidate;
                        break;
                    }
                }
            }
            bestPathRow = region.pathRow;
            bestSpan = span;
        }
    }

    return best;
}

int LadderBranchLogic::innermostInputRegionIndex(const LadderRung &rung)
{
    LadderRung copy = rung;
    migrateLegacyInputBranch(&copy);
    if (copy.inputBranchRegions.isEmpty()) {
        return -1;
    }

    int bestIndex = 0;
    int bestPathRow = copy.inputBranchRegions.first().pathRow;
    int bestSpan = copy.inputBranchRegions.first().joinCol - copy.inputBranchRegions.first().forkCol;

    for (int i = 1; i < copy.inputBranchRegions.size(); ++i) {
        const LadderInputBranchRegion &region = copy.inputBranchRegions[i];
        const int span = region.joinCol - region.forkCol;
        if (region.pathRow > bestPathRow || (region.pathRow == bestPathRow && span < bestSpan)) {
            bestIndex = i;
            bestPathRow = region.pathRow;
            bestSpan = span;
        }
    }

    if (!rung.inputBranchRegions.isEmpty() && bestIndex < rung.inputBranchRegions.size()) {
        return bestIndex;
    }

    return bestIndex;
}

int LadderBranchLogic::inputBranchRegionIndex(const LadderRung &rung, const LadderInputBranchRegion &region)
{
    for (int i = 0; i < rung.inputBranchRegions.size(); ++i) {
        const LadderInputBranchRegion &candidate = rung.inputBranchRegions[i];
        if (candidate.forkCol == region.forkCol && candidate.joinCol == region.joinCol
            && candidate.pathRow == region.pathRow) {
            return i;
        }
    }
    return -1;
}

int LadderBranchLogic::inferInputBranchRegionFromElements(const LadderRung &rung,
                                                          const QVector<int> &localIds)
{
    if (localIds.isEmpty()) {
        return -1;
    }

    LadderRung copy = rung;
    migrateLegacyInputBranch(&copy);
    if (copy.inputBranchRegions.isEmpty()) {
        return -1;
    }

    QSet<int> ids(localIds.begin(), localIds.end());
    int bestIndex = -1;
    int bestPathRow = -1;
    int bestSpan = std::numeric_limits<int>::max();

    for (int i = 0; i < copy.inputBranchRegions.size(); ++i) {
        const LadderInputBranchRegion &region = copy.inputBranchRegions[i];
        bool allInside = true;
        int matchedSelected = 0;

        for (const LadderElement &element : rung.elements) {
            if (!ids.contains(element.localId) || element.kind == LadderElementKind::Coil) {
                continue;
            }
            ++matchedSelected;
            if (element.col < region.forkCol || element.col >= region.joinCol || element.row < region.pathRow) {
                allInside = false;
                break;
            }
        }

        if (matchedSelected == 0 || !allInside) {
            continue;
        }

        const int span = region.joinCol - region.forkCol;
        if (region.pathRow > bestPathRow || (region.pathRow == bestPathRow && span < bestSpan)) {
            bestIndex = i;
            bestPathRow = region.pathRow;
            bestSpan = span;
        }
    }

    return bestIndex;
}

bool LadderBranchLogic::isRootInputRegion(const LadderInputBranchRegion &region,
                                          const QVector<LadderInputBranchRegion> &regions)
{
    for (const LadderInputBranchRegion &other : regions) {
        if (regionNestContains(other, region)) {
            return false;
        }
    }
    return true;
}

int LadderBranchLogic::maxParallelRowForRegion(const LadderRung &rung,
                                               const LadderInputBranchRegion &region)
{
    int maxRow = region.pathRow;
    for (const LadderElement &element : rung.elements) {
        if (element.kind == LadderElementKind::Coil) {
            continue;
        }
        if (element.row <= region.pathRow) {
            continue;
        }
        if (element.col < region.forkCol || element.col >= region.joinCol) {
            continue;
        }
        maxRow = qMax(maxRow, element.row);
    }

    for (const LadderInputBranchRegion &other : rung.inputBranchRegions) {
        if (regionNestContains(region, other)) {
            maxRow = qMax(maxRow, maxParallelRowForRegion(rung, other));
        }
    }

    return maxRow;
}

const LadderInputBranchRegion *LadderBranchLogic::nestedInputRegionOnPathRow(const LadderRung &rung,
                                                                            const LadderInputBranchRegion &parent)
{
    const LadderInputBranchRegion *best = nullptr;
    int bestSpan = std::numeric_limits<int>::max();

    for (const LadderInputBranchRegion &region : rung.inputBranchRegions) {
        if (region.pathRow != parent.pathRow) {
            continue;
        }
        if (!regionNestContains(parent, region)) {
            continue;
        }
        if (region.forkCol <= parent.forkCol && region.joinCol >= parent.joinCol) {
            continue;
        }
        const int span = region.joinCol - region.forkCol;
        if (span < bestSpan) {
            bestSpan = span;
            best = &region;
        }
    }

    return best;
}

const LadderInputBranchRegion *LadderBranchLogic::childInputRegionOnRow(const LadderRung &rung,
                                                                        const LadderInputBranchRegion &parent,
                                                                        int row)
{
    const LadderInputBranchRegion *best = nullptr;
    int bestSpan = std::numeric_limits<int>::max();

    for (const LadderInputBranchRegion &region : rung.inputBranchRegions) {
        if (region.pathRow != row) {
            continue;
        }
        if (!regionNestContains(parent, region)) {
            continue;
        }
        const int span = region.joinCol - region.forkCol;
        if (span < bestSpan) {
            bestSpan = span;
            best = &region;
        }
    }

    return best;
}

QVector<const LadderInputBranchRegion *> LadderBranchLogic::childInputRegions(const LadderRung &rung,
                                                                              const LadderInputBranchRegion &parent)
{
    QVector<const LadderInputBranchRegion *> children;
    for (const LadderInputBranchRegion &region : rung.inputBranchRegions) {
        if (region.pathRow <= parent.pathRow) {
            continue;
        }
        if (regionNestContains(parent, region)) {
            children.push_back(&region);
        }
    }
    return children;
}

QVector<const LadderInputBranchRegion *> LadderBranchLogic::rootInputRegions(const LadderRung &rung)
{
    QVector<const LadderInputBranchRegion *> roots;
    for (const LadderInputBranchRegion &region : rung.inputBranchRegions) {
        if (isRootInputRegion(region, rung.inputBranchRegions)) {
            roots.push_back(&region);
        }
    }
    std::sort(roots.begin(), roots.end(), [](const LadderInputBranchRegion *a, const LadderInputBranchRegion *b) {
        if (a->pathRow != b->pathRow) {
            return a->pathRow < b->pathRow;
        }
        return a->forkCol < b->forkCol;
    });
    return roots;
}

namespace {

const LadderInputBranchRegion *matchingRegion(const LadderRung &rung, const LadderInputBranchRegion &key)
{
    for (const LadderInputBranchRegion &region : rung.inputBranchRegions) {
        if (region.forkCol == key.forkCol && region.joinCol == key.joinCol && region.pathRow == key.pathRow) {
            return &region;
        }
    }
    return nullptr;
}

bool elementInRegionParallelLeg(const LadderRung &rung,
                                const LadderInputBranchRegion &region,
                                const LadderElement &element)
{
    if (element.kind == LadderElementKind::Coil) {
        return false;
    }
    if (element.row <= region.pathRow || element.col < region.forkCol) {
        return false;
    }

    int outputForkCol = 0;
    if (rung.hasOutputBranch() && rung.outputBranchForkColumn(&outputForkCol)
        && element.col >= outputForkCol) {
        return false;
    }

    const LadderInputBranchRegion *owner = LadderBranchLogic::inputRegionForCell(rung, element.row, element.col);
    if (owner != nullptr) {
        if (owner->forkCol == region.forkCol && owner->joinCol == region.joinCol
            && owner->pathRow == region.pathRow) {
            return true;
        }
        return LadderBranchLogic::regionNestContains(region, *owner);
    }

    const LadderInputBranchRegion *legRegion = nullptr;
    int bestPathRow = -1;
    for (const LadderInputBranchRegion &candidate : rung.inputBranchRegions) {
        if (element.row <= candidate.pathRow || element.col < candidate.forkCol) {
            continue;
        }
        if (candidate.pathRow >= bestPathRow) {
            legRegion = &candidate;
            bestPathRow = candidate.pathRow;
        }
    }
    return legRegion == &region;
}

} // namespace

void LadderBranchLogic::syncInputBranchJoinCols(LadderRung *rung)
{
    if (rung == nullptr) {
        return;
    }
    migrateLegacyInputBranch(rung);
    if (rung->inputBranchRegions.isEmpty()) {
        return;
    }

    for (LadderInputBranchRegion &region : rung->inputBranchRegions) {
        int parallelMaxCol = region.forkCol;
        for (const LadderElement &element : rung->elements) {
            if (elementInRegionParallelLeg(*rung, region, element)) {
                parallelMaxCol = qMax(parallelMaxCol, element.col);
            }
        }

        int outputForkCol = 0;
        const bool hasOutput = rung->hasOutputBranch() && rung->outputBranchForkColumn(&outputForkCol);
        const int carrierUpperBound = hasOutput ? outputForkCol : std::numeric_limits<int>::max();

        int carrierMaxCol = region.forkCol - 1;
        int nextCol = region.forkCol;
        QVector<int> carrierCols;
        for (const LadderElement &element : rung->elements) {
            if (element.kind == LadderElementKind::Coil) {
                continue;
            }
            if (element.row != region.pathRow || element.col < region.forkCol) {
                continue;
            }
            if (element.col >= carrierUpperBound) {
                continue;
            }
            carrierCols.push_back(element.col);
        }
        std::sort(carrierCols.begin(), carrierCols.end());
        carrierCols.erase(std::unique(carrierCols.begin(), carrierCols.end()), carrierCols.end());
        for (int col : carrierCols) {
            if (col == nextCol) {
                carrierMaxCol = col;
                ++nextCol;
            } else if (col > nextCol) {
                break;
            }
        }

        region.joinCol = region.forkCol + 1;
        region.joinCol = qMax(region.joinCol, parallelMaxCol + 1);
        if (carrierMaxCol >= region.forkCol) {
            region.joinCol = qMax(region.joinCol, carrierMaxCol + 1);
        }
    }

    for (LadderInputBranchRegion &parent : rung->inputBranchRegions) {
        for (const LadderInputBranchRegion &child : rung->inputBranchRegions) {
            if (&parent == &child) {
                continue;
            }
            if (regionNestContains(parent, child)) {
                parent.joinCol = qMax(parent.joinCol, child.joinCol);
            }
        }
    }

    syncLegacyInputBranchFields(rung);
}

const LadderInputBranchRegion *LadderBranchLogic::regionForBranchPlacement(const LadderRung &rung,
                                                                           int row,
                                                                           int col)
{
    LadderRung copy = rung;
    migrateLegacyInputBranch(&copy);

    const LadderInputBranchRegion *found = inputRegionForCell(rung, row, col);
    if (found != nullptr) {
        return found;
    }

    LadderInputBranchRegion bestKey;
    bool haveBest = false;
    int bestPathRow = -1;
    int bestSpan = std::numeric_limits<int>::max();

    for (const LadderInputBranchRegion &region : copy.inputBranchRegions) {
        if (row == region.pathRow && col >= region.forkCol && col <= region.joinCol) {
            bool hasCarrier = false;
            for (const LadderElement &element : rung.elements) {
                if (element.kind == LadderElementKind::Coil) {
                    continue;
                }
                if (element.row == region.pathRow && element.col >= region.forkCol
                    && element.col < region.joinCol) {
                    hasCarrier = true;
                    break;
                }
            }
            if (hasCarrier) {
                bestKey = region;
                haveBest = true;
                bestPathRow = region.pathRow;
                bestSpan = region.joinCol - region.forkCol;
                break;
            }
        }
    }

    if (row > 0) {
        for (const LadderInputBranchRegion &region : copy.inputBranchRegions) {
            if (row <= region.pathRow || col < region.forkCol) {
                continue;
            }
            const int span = region.joinCol - region.forkCol;
            if (!haveBest || region.pathRow > bestPathRow
                || (region.pathRow == bestPathRow && span < bestSpan)) {
                bestKey = region;
                haveBest = true;
                bestPathRow = region.pathRow;
                bestSpan = span;
            }
        }
    }

    if (!haveBest) {
        return nullptr;
    }
    return matchingRegion(rung, bestKey);
}

int LadderBranchLogic::maxInsertColForBranchRow(const LadderRung &rung,
                                                const LadderInputBranchRegion &region,
                                                int row)
{
    int maxCol = region.forkCol - 1;
    for (const LadderElement &element : rung.elements) {
        if (element.kind == LadderElementKind::Coil || element.row != row) {
            continue;
        }
        if (element.col < region.forkCol) {
            continue;
        }
        if (row == region.pathRow && element.col >= region.joinCol) {
            continue;
        }
        maxCol = qMax(maxCol, element.col);
    }
    return maxCol + 1;
}

int LadderBranchLogic::coilColOnRow(const LadderRung &rung, int row)
{
    for (const LadderElement &element : rung.elements) {
        if (element.kind == LadderElementKind::Coil && element.row == row) {
            return element.col;
        }
    }
    return -1;
}

bool LadderBranchLogic::outputBranchInstructionSpan(const LadderRung &rung,
                                                    int row,
                                                    int *forkCol,
                                                    int *maxInsertCol)
{
    if (!rung.hasOutputBranch()) {
        return false;
    }

    int outputForkCol = 0;
    if (!rung.outputBranchForkColumn(&outputForkCol)) {
        return false;
    }

    const int coilCol = coilColOnRow(rung, row);
    if (coilCol < 0) {
        return false;
    }

    if (forkCol != nullptr) {
        *forkCol = outputForkCol;
    }
    if (maxInsertCol != nullptr) {
        *maxInsertCol = coilCol;
    }
    return true;
}
