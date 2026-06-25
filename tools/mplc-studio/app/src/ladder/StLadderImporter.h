/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderModel.h"

#include <QStringList>

class StLadderImporter
{
public:
    struct Result {
        bool ok = false;
        LadderProgram program;
        QStringList diagnostics;
    };

    static Result importSource(const QString &source);
};
