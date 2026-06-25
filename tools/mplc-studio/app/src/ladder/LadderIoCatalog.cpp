/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderIoCatalog.h"

namespace {

QVector<LadderIoPoint> buildModDev70Points()
{
    QVector<LadderIoPoint> points;
    points.reserve(16);

    for (int i = 1; i <= 8; ++i) {
        LadderIoPoint dip;
        dip.name = QStringLiteral("Dip%1").arg(i);
        dip.ioAddress = QStringLiteral("%1IX0.%2").arg(QLatin1Char('%')).arg(i - 1);
        dip.roleLabel = QStringLiteral("DIP %1").arg(i);
        dip.type = LadderVarType::Bool;
        dip.isInput = true;
        points.push_back(dip);

        LadderIoPoint led;
        led.name = QStringLiteral("Led%1").arg(i);
        led.ioAddress = QStringLiteral("%1QX0.%2").arg(QLatin1Char('%')).arg(i - 1);
        led.roleLabel = QStringLiteral("LED %1").arg(i);
        led.type = LadderVarType::Bool;
        led.isInput = false;
        points.push_back(led);
    }

    return points;
}

} // namespace

const QVector<LadderIoPoint> &LadderIoCatalog::modDev70Points()
{
    static const QVector<LadderIoPoint> points = buildModDev70Points();
    return points;
}

const LadderIoPoint *LadderIoCatalog::findByName(const QString &name)
{
    for (const LadderIoPoint &point : modDev70Points()) {
        if (point.name.compare(name, Qt::CaseInsensitive) == 0) {
            return &point;
        }
    }
    return nullptr;
}

void LadderIoCatalog::ensureModDev70Variables(LadderProgram &program)
{
    for (const LadderIoPoint &point : modDev70Points()) {
        bool exists = false;
        for (const LadderVariable &var : program.variables) {
            if (var.name.compare(point.name, Qt::CaseInsensitive) == 0) {
                exists = true;
                break;
            }
        }
        if (exists) {
            continue;
        }

        LadderVariable var;
        var.name = point.name;
        var.type = point.type;
        var.ioAddress = point.ioAddress;
        program.variables.push_back(var);
    }
}

QString LadderIoCatalog::inputNameForRung(int rungIndex)
{
    const int channel = (qMax(0, rungIndex) % 8) + 1;
    return QStringLiteral("Dip%1").arg(channel);
}

QString LadderIoCatalog::inputNameForBranchLeg(int rungIndex, int row)
{
    const int channel = (qMax(0, rungIndex + qMax(0, row)) % 8) + 1;
    return QStringLiteral("Dip%1").arg(channel);
}

QString LadderIoCatalog::outputNameForRung(int rungIndex)
{
    const int channel = (qMax(0, rungIndex) % 8) + 1;
    return QStringLiteral("Led%1").arg(channel);
}

QString LadderIoCatalog::outputNameForBranchCoil(int rungIndex, int branchIndex)
{
    const int channel = (qMax(0, rungIndex + qMax(0, branchIndex)) % 8) + 1;
    return QStringLiteral("Led%1").arg(channel);
}

QString LadderIoCatalog::ioDisplayLabel(const LadderVariable &variable)
{
    if (const LadderIoPoint *point = findByName(variable.name)) {
        return point->roleLabel;
    }
    return variable.ioAddress;
}
