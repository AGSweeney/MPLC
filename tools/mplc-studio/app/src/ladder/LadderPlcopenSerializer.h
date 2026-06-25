/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderModel.h"

class LadderPlcopenSerializer
{
public:
    static bool load(const QString &path, LadderProgram &program, QString *errorMessage);
    static bool save(const QString &path, const LadderProgram &program, QString *errorMessage);
};
