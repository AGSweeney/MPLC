/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderModel.h"

#include <QStringList>

class LadderValidator
{
public:
    struct Issue {
        enum class Severity { Warning, Error };
        Severity severity = Severity::Warning;
        int rungIndex = -1;
        int localId = -1;
        QString message;
    };

    static QVector<Issue> validate(const LadderProgram &program);
};
