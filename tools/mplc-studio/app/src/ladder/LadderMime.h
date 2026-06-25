/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QString>

namespace LadderMime {

inline QString mimeType()
{
    return QStringLiteral("application/x-mplc-ladder-element");
}

inline QString contactPayload(bool negated)
{
    return negated ? QStringLiteral("contact/nc") : QStringLiteral("contact/no");
}

inline QString coilPayload(int kind)
{
    switch (kind) {
    case 1:
        return QStringLiteral("coil/set");
    case 2:
        return QStringLiteral("coil/reset");
    default:
        return QStringLiteral("coil/normal");
    }
}

inline QString functionBlockPayload(const QString &typeName)
{
    return QStringLiteral("fb/") + typeName;
}

} // namespace LadderMime
