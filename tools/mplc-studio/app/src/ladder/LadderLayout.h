/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderElementItem.h"

#include <QtGlobal>

namespace LadderLayout {

constexpr qreal kLeftRailX = 56.0;
constexpr qreal kRungOriginX = 72.0;
constexpr qreal kRightRailPad = 24.0;
constexpr qreal kTopMargin = 24.0;
constexpr qreal kRungSpacing = 12.0;
constexpr qreal kMinLayoutWidth = 800.0;

inline qreal columnX(int col)
{
    return kRungOriginX + (col - 1) * LadderElementItem::kCellWidth;
}

inline int columnAtX(qreal x)
{
    const qreal rel = x - kRungOriginX;
    if (rel < 0) {
        return 1;
    }
    return qMax(1, static_cast<int>(rel / LadderElementItem::kCellWidth) + 1);
}

inline int rowAtY(qreal yOffset, qreal y)
{
    const qreal rel = y - yOffset;
    if (rel < 0) {
        return 0;
    }
    return qMax(0, static_cast<int>(rel / LadderElementItem::kCellHeight));
}

inline qreal rowBusY(qreal yOffset, int row)
{
    return yOffset + row * LadderElementItem::kCellHeight + LadderElementItem::kBusLineY;
}

inline int outputCoilColumn(qreal layoutWidth)
{
    const qreal coilLeft = layoutWidth - kRightRailPad - LadderElementItem::kCellWidth;
    return qMax(2, columnAtX(coilLeft));
}

} // namespace LadderLayout
