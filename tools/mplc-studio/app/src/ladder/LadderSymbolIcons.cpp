/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderSymbolIcons.h"

#include <QPainter>
#include <QPixmap>

namespace LadderSymbolIcons {

namespace {

constexpr int kIconWidth = 52;
constexpr int kIconHeight = 26;

void paintContact(QPainter &painter, bool negated)
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    const qreal midY = kIconHeight / 2.0;
    const qreal leftX = 12;
    const qreal rightX = kIconWidth - 12;

    QPen wirePen(QColor("#101010"), 2.0, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(wirePen);
    painter.drawLine(QPointF(2, midY), QPointF(leftX, midY));
    painter.drawLine(QPointF(leftX, midY - 10), QPointF(leftX, midY + 10));
    painter.drawLine(QPointF(rightX, midY - 10), QPointF(rightX, midY + 10));
    painter.drawLine(QPointF(rightX, midY), QPointF(kIconWidth - 2, midY));

    if (negated) {
        QPen slashPen(QColor("#c0392b"), 2.0, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(slashPen);
        painter.drawLine(QPointF(leftX - 1, midY + 10), QPointF(rightX + 1, midY - 10));
    }
}

void paintCoil(QPainter &painter, LadderSymbolKind kind)
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    const qreal midY = kIconHeight / 2.0;
    const qreal arcRadius = 9;
    const qreal centerX = kIconWidth / 2.0;
    const QRectF leftArc(centerX - arcRadius * 2 - 1, midY - arcRadius, arcRadius * 2, arcRadius * 2);
    const QRectF rightArc(centerX + 1, midY - arcRadius, arcRadius * 2, arcRadius * 2);

    QPen wirePen(QColor("#101010"), 2.0, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(wirePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(QPointF(2, midY), QPointF(leftArc.left(), midY));
    painter.drawArc(leftArc, 90 * 16, 180 * 16);
    painter.drawArc(rightArc, 90 * 16, -180 * 16);
    painter.drawLine(QPointF(rightArc.right(), midY), QPointF(kIconWidth - 2, midY));

    if (kind == LadderSymbolKind::CoilSet || kind == LadderSymbolKind::CoilReset) {
        QFont markFont = painter.font();
        markFont.setBold(true);
        markFont.setPointSize(7);
        painter.setFont(markFont);
        painter.setPen(QColor("#1a1a1a"));
        const QString mark = kind == LadderSymbolKind::CoilSet ? QStringLiteral("S") : QStringLiteral("R");
        painter.drawText(QRectF(centerX - 8, midY - 8, 16, 16), Qt::AlignCenter, mark);
    }
}

void paintFunctionBlock(QPainter &painter, const QString &typeName)
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    const qreal midY = kIconHeight / 2.0;
    const QRectF rect(8, 3, kIconWidth - 16, kIconHeight - 6);

    painter.setPen(QPen(QColor("#2d325a"), 1.5));
    painter.setBrush(QColor("#f7f9fc"));
    painter.drawRect(rect);

    QFont blockFont = painter.font();
    blockFont.setBold(true);
    blockFont.setPointSize(8);
    painter.setFont(blockFont);
    painter.setPen(Qt::black);
    painter.drawText(rect, Qt::AlignCenter, typeName);

    QPen wirePen(QColor("#101010"), 2.0, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(wirePen);
    painter.drawLine(QPointF(2, midY), QPointF(rect.left(), midY));
    painter.drawLine(QPointF(rect.right(), midY), QPointF(kIconWidth - 2, midY));
}

void paintRungLine(QPainter &painter, bool withPlus)
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    const qreal midY = kIconHeight / 2.0;
    QPen wirePen(QColor("#101010"), 2.0, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(wirePen);
    painter.drawLine(QPointF(2, midY), QPointF(kIconWidth - 2, midY));
    if (withPlus) {
        painter.drawLine(QPointF(kIconWidth / 2.0, midY - 6), QPointF(kIconWidth / 2.0, midY + 6));
        painter.drawLine(QPointF(kIconWidth / 2.0 - 6, midY), QPointF(kIconWidth / 2.0 + 6, midY));
    }
}

void paintBranchFork(QPainter &painter, bool remove)
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    const qreal midY = 8.0;
    const qreal forkX = 14.0;
    const qreal joinX = kIconWidth - 10.0;
    const qreal lowerY = kIconHeight - 4.0;

    QPen wirePen(QColor("#101010"), 2.0, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(wirePen);
    painter.drawLine(QPointF(2, midY), QPointF(forkX, midY));
    painter.drawLine(QPointF(forkX, midY), QPointF(joinX, midY));
    painter.drawLine(QPointF(forkX, midY), QPointF(forkX, lowerY));
    painter.drawLine(QPointF(forkX, lowerY), QPointF(joinX, lowerY));
    painter.drawLine(QPointF(joinX, lowerY), QPointF(joinX, midY));
    painter.drawLine(QPointF(joinX, midY), QPointF(kIconWidth - 2, midY));

    if (remove) {
        QPen xPen(QColor("#c0392b"), 2.0, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(xPen);
        const qreal cx = (forkX + joinX) / 2.0;
        const qreal cy = (midY + lowerY) / 2.0;
        painter.drawLine(QPointF(cx - 5, cy - 4), QPointF(cx + 5, cy + 4));
        painter.drawLine(QPointF(cx + 5, cy - 4), QPointF(cx - 5, cy + 4));
    }
}

QPixmap renderSymbol(LadderSymbolKind kind, const QString &fbTypeName)
{
    QPixmap pixmap(kIconWidth, kIconHeight);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    switch (kind) {
    case LadderSymbolKind::ContactNo:
        paintContact(painter, false);
        break;
    case LadderSymbolKind::ContactNc:
        paintContact(painter, true);
        break;
    case LadderSymbolKind::CoilNormal:
        paintCoil(painter, kind);
        break;
    case LadderSymbolKind::CoilSet:
    case LadderSymbolKind::CoilReset:
        paintCoil(painter, kind);
        break;
    case LadderSymbolKind::FunctionBlock:
        paintFunctionBlock(painter, fbTypeName);
        break;
    case LadderSymbolKind::AddRung:
        paintRungLine(painter, true);
        break;
    case LadderSymbolKind::SurroundBranch:
        paintBranchFork(painter, false);
        break;
    case LadderSymbolKind::RemoveBranch:
        paintBranchFork(painter, true);
        break;
    }
    painter.end();
    return pixmap;
}

} // namespace

QIcon icon(LadderSymbolKind kind, const QString &fbTypeName)
{
    return QIcon(renderSymbol(kind, fbTypeName));
}

} // namespace LadderSymbolIcons
