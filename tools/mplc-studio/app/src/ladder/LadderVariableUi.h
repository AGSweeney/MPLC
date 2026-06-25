/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderModel.h"

class QComboBox;

namespace LadderVariableUi {

void populateVariableChoices(QComboBox *combo, const LadderProgram *program, LadderElementKind kind);
void populateTimeVariableChoices(QComboBox *combo, const LadderProgram *program);
int comboIndexForVariable(QComboBox *combo, const QString &variableName);
QString selectedVariableName(QComboBox *combo);
void selectVariable(QComboBox *combo, const QString &variableName);

} // namespace LadderVariableUi
