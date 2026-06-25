/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderModel.h"

class QWidget;

class LadderElementEditorDialog
{
public:
    static bool editElement(LadderProgram *program, LadderElement *element, QWidget *parent);
};
