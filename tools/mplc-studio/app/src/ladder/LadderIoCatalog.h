/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderModel.h"

struct LadderIoPoint {
    QString name;
    QString ioAddress;
    QString roleLabel;
    LadderVarType type = LadderVarType::Bool;
    bool isInput = true;
};

class LadderIoCatalog
{
public:
    static const QVector<LadderIoPoint> &modDev70Points();
    static const LadderIoPoint *findByName(const QString &name);
    static void ensureModDev70Variables(LadderProgram &program);
    static QString inputNameForRung(int rungIndex);
    static QString inputNameForBranchLeg(int rungIndex, int row);
    static QString outputNameForRung(int rungIndex);
    static QString outputNameForBranchCoil(int rungIndex, int branchIndex);
    static QString ioDisplayLabel(const LadderVariable &variable);
};
