/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QVector>

struct LadderRung;

struct LadderInputBranchRegion {
    int forkCol = 0;
    int joinCol = 0;
    int pathRow = 0;
};

class LadderBranchLogic
{
public:
    static void migrateLegacyInputBranch(LadderRung *rung);
    static void syncLegacyInputBranchFields(LadderRung *rung);
    static QVector<LadderInputBranchRegion> inputBranchRegions(const LadderRung &rung);

    static bool regionNestContains(const LadderInputBranchRegion &parent,
                                   const LadderInputBranchRegion &child);
    static const LadderInputBranchRegion *inputRegionForCell(const LadderRung &rung, int row, int col);
    static int innermostInputRegionIndex(const LadderRung &rung);
    static int inputBranchRegionIndex(const LadderRung &rung, const LadderInputBranchRegion &region);
    static int inferInputBranchRegionFromElements(const LadderRung &rung, const QVector<int> &localIds);
    static bool isRootInputRegion(const LadderInputBranchRegion &region,
                                  const QVector<LadderInputBranchRegion> &regions);
    static int maxParallelRowForRegion(const LadderRung &rung, const LadderInputBranchRegion &region);
    static const LadderInputBranchRegion *nestedInputRegionOnPathRow(const LadderRung &rung,
                                                                      const LadderInputBranchRegion &parent);
    static const LadderInputBranchRegion *childInputRegionOnRow(const LadderRung &rung,
                                                                 const LadderInputBranchRegion &parent,
                                                                 int row);
    static QVector<const LadderInputBranchRegion *> childInputRegions(const LadderRung &rung,
                                                                       const LadderInputBranchRegion &parent);
    static QVector<const LadderInputBranchRegion *> rootInputRegions(const LadderRung &rung);
    static void syncInputBranchJoinCols(LadderRung *rung);
    static const LadderInputBranchRegion *regionForBranchPlacement(const LadderRung &rung, int row, int col);
    static int maxInsertColForBranchRow(const LadderRung &rung, const LadderInputBranchRegion &region, int row);
    static int coilColOnRow(const LadderRung &rung, int row);
    static bool outputBranchInstructionSpan(const LadderRung &rung, int row, int *forkCol, int *maxInsertCol);
};
