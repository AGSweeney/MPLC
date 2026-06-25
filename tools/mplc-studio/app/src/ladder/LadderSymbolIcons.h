/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QIcon>
#include <QString>

enum class LadderSymbolKind {
    ContactNo,
    ContactNc,
    CoilNormal,
    CoilSet,
    CoilReset,
    FunctionBlock,
    AddRung,
    SurroundBranch,
    RemoveBranch
};

namespace LadderSymbolIcons {

QIcon icon(LadderSymbolKind kind, const QString &fbTypeName = QString());

} // namespace LadderSymbolIcons
