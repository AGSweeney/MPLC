/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderModel.h"

#include <QStringList>

class LadderStExporter
{
public:
    struct Result {
        bool ok = false;
        QString source;
        QStringList diagnostics;
    };

    static Result exportProgram(const LadderProgram &program);
};
