/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderStExporter.h"
#include "LadderBranchLogic.h"
#include "LadderModel.h"
#include "LadderValidator.h"

#include <QMap>
#include <QSet>
#include <QVector>
#include <functional>

namespace {

QString varRef(const QString &name)
{
    return name;
}

QString boolLiteral(bool value)
{
    return value ? QStringLiteral("TRUE") : QStringLiteral("FALSE");
}

QString contactExpr(const LadderElement &element)
{
    const QString ref = varRef(element.variable);
    return element.negated ? QStringLiteral("NOT %1").arg(ref) : ref;
}

QString sanitizeInstanceName(const QString &typeName, int localId)
{
    return QStringLiteral("%1_%2").arg(typeName.toLower(), QString::number(localId));
}

QString pinAssignment(const LadderPinBinding &binding)
{
    if (!binding.variable.isEmpty()) {
        return QStringLiteral("%1 := %2").arg(binding.formalName, varRef(binding.variable));
    }
    if (!binding.literal.isEmpty()) {
        return QStringLiteral("%1 := %2").arg(binding.formalName, binding.literal);
    }
    return QString();
}

QVector<int> incomingNodes(const LadderRung &rung, int localId)
{
    QVector<int> incoming;
    for (const LadderConnection &connection : rung.connections) {
        if (connection.toLocalId == localId) {
            incoming.push_back(connection.fromLocalId);
        }
    }
    return incoming;
}

QVector<int> outgoingNodes(const LadderRung &rung, int localId)
{
    QVector<int> outgoing;
    for (const LadderConnection &connection : rung.connections) {
        if (connection.fromLocalId == localId) {
            outgoing.push_back(connection.toLocalId);
        }
    }
    return outgoing;
}

const LadderElement *findCoil(const LadderRung &rung)
{
    for (const LadderElement &element : rung.elements) {
        if (element.kind == LadderElementKind::Coil) {
            return &element;
        }
    }
    return nullptr;
}

QString buildFeedExpr(const LadderRung &rung,
                      int nodeId,
                      const QMap<int, const LadderElement *> &byId,
                      QSet<int> &visiting)
{
    if (visiting.contains(nodeId)) {
        return boolLiteral(false);
    }
    visiting.insert(nodeId);

    const LadderElement *element = byId.value(nodeId, nullptr);
    if (element == nullptr) {
        visiting.remove(nodeId);
        return boolLiteral(true);
    }

    if (element->kind == LadderElementKind::Coil) {
        visiting.remove(nodeId);
        return boolLiteral(true);
    }

    if (element->kind == LadderElementKind::Contact) {
        visiting.remove(nodeId);
        return contactExpr(*element);
    }

    const QVector<int> incoming = incomingNodes(rung, nodeId);
    if (incoming.isEmpty()) {
        visiting.remove(nodeId);
        return boolLiteral(true);
    }

    QStringList parts;
    for (int sourceId : incoming) {
        parts.push_back(buildFeedExpr(rung, sourceId, byId, visiting));
    }
    visiting.remove(nodeId);

    if (parts.isEmpty()) {
        return boolLiteral(true);
    }
    return parts.join(QStringLiteral(" AND "));
}

QString buildPathExpr(const LadderRung &rung,
                      int nodeId,
                      const QMap<int, const LadderElement *> &byId,
                      QSet<int> &visiting)
{
    if (visiting.contains(nodeId)) {
        return boolLiteral(false);
    }
    visiting.insert(nodeId);

    const LadderElement *element = byId.value(nodeId, nullptr);
    if (element == nullptr) {
        visiting.remove(nodeId);
        return boolLiteral(true);
    }

    QString nodeExpr;
    if (element->kind == LadderElementKind::Contact) {
        nodeExpr = contactExpr(*element);
    } else if (element->kind == LadderElementKind::FunctionBlock) {
        const QString instance = sanitizeInstanceName(element->fbTypeName, element->localId);
        nodeExpr = QStringLiteral("%1.Q").arg(instance);
    } else if (element->kind == LadderElementKind::Coil) {
        visiting.remove(nodeId);
        return boolLiteral(true);
    } else {
        nodeExpr = boolLiteral(true);
    }

    const QVector<int> incoming = incomingNodes(rung, nodeId);
    if (incoming.isEmpty()) {
        visiting.remove(nodeId);
        return nodeExpr;
    }

    QStringList parts;
    for (int sourceId : incoming) {
        parts.push_back(buildPathExpr(rung, sourceId, byId, visiting));
    }
    visiting.remove(nodeId);

    if (parts.isEmpty()) {
        return nodeExpr;
    }

    return QStringLiteral("(%1) AND %2").arg(parts.join(QStringLiteral(" AND ")), nodeExpr);
}

using AndElementsFn = std::function<QString(const QVector<const LadderElement *> &)>;

QString buildInputRegionSpanExpr(const LadderRung &rung,
                                 const LadderInputBranchRegion &region,
                                 const AndElementsFn &andElements)
{
    QStringList orPaths;

    const LadderInputBranchRegion *nested = LadderBranchLogic::nestedInputRegionOnPathRow(rung, region);
    if (nested != nullptr) {
        QVector<const LadderElement *> prefix;
        for (const LadderElement &element : rung.elements) {
            if (element.row == region.pathRow && element.kind != LadderElementKind::Coil &&
                element.col >= region.forkCol && element.col < nested->forkCol) {
                prefix.push_back(&element);
            }
        }
        std::sort(prefix.begin(), prefix.end(), [](const LadderElement *a, const LadderElement *b) {
            return a->col < b->col;
        });

        QVector<const LadderElement *> suffix;
        for (const LadderElement &element : rung.elements) {
            if (element.row == region.pathRow && element.kind != LadderElementKind::Coil &&
                element.col >= nested->joinCol && element.col < region.joinCol) {
                suffix.push_back(&element);
            }
        }
        std::sort(suffix.begin(), suffix.end(), [](const LadderElement *a, const LadderElement *b) {
            return a->col < b->col;
        });

        QStringList series;
        const QString prefixExpr = andElements(prefix);
        if (prefixExpr != boolLiteral(true)) {
            series.push_back(prefixExpr);
        }
        series.push_back(QStringLiteral("(%1)").arg(buildInputRegionSpanExpr(rung, *nested, andElements)));
        const QString suffixExpr = andElements(suffix);
        if (suffixExpr != boolLiteral(true)) {
            series.push_back(suffixExpr);
        }
        orPaths.push_back(series.join(QStringLiteral(" AND ")));
    } else {
        QVector<const LadderElement *> spanMain;
        for (const LadderElement &element : rung.elements) {
            if (element.row == region.pathRow && element.kind != LadderElementKind::Coil &&
                element.col >= region.forkCol && element.col < region.joinCol) {
                spanMain.push_back(&element);
            }
        }
        std::sort(spanMain.begin(), spanMain.end(), [](const LadderElement *a, const LadderElement *b) {
            return a->col < b->col;
        });
        if (!spanMain.isEmpty()) {
            orPaths.push_back(andElements(spanMain));
        }
    }

    const int maxRow = LadderBranchLogic::maxParallelRowForRegion(rung, region);
    for (int row = region.pathRow + 1; row <= maxRow; ++row) {
        const LadderInputBranchRegion *child = LadderBranchLogic::childInputRegionOnRow(rung, region, row);
        if (child != nullptr) {
            orPaths.push_back(buildInputRegionSpanExpr(rung, *child, andElements));
        } else {
            QVector<const LadderElement *> branchSpan;
            for (const LadderElement &element : rung.elements) {
                if (element.row == row && element.kind != LadderElementKind::Coil &&
                    element.col >= region.forkCol && element.col < region.joinCol) {
                    branchSpan.push_back(&element);
                }
            }
            std::sort(branchSpan.begin(), branchSpan.end(), [](const LadderElement *a, const LadderElement *b) {
                return a->col < b->col;
            });
            if (!branchSpan.isEmpty()) {
                orPaths.push_back(andElements(branchSpan));
            }
        }
    }

    return orPaths.isEmpty() ? boolLiteral(true) : orPaths.join(QStringLiteral(" OR "));
}

QString buildForkJoinCondition(const LadderRung &rung, const LadderElement &coil, int forkCol, int joinCol)
{
    QMap<int, const LadderElement *> byId;
    for (const LadderElement &element : rung.elements) {
        byId.insert(element.localId, &element);
    }

    auto andElements = [&](const QVector<const LadderElement *> &elements) -> QString {
        QStringList parts;
        for (const LadderElement *element : elements) {
            if (element->kind == LadderElementKind::Contact) {
                parts.push_back(contactExpr(*element));
            } else if (element->kind == LadderElementKind::FunctionBlock) {
                parts.push_back(QStringLiteral("%1.Q")
                                    .arg(sanitizeInstanceName(element->fbTypeName, element->localId)));
            }
        }
        if (parts.isEmpty()) {
            return boolLiteral(true);
        }
        return parts.join(QStringLiteral(" AND "));
    };

    QVector<const LadderElement *> prefix;
    QVector<const LadderElement *> suffix;
    for (const LadderElement &element : rung.elements) {
        if (element.kind == LadderElementKind::Coil || element.row != 0) {
            continue;
        }
        if (element.col < forkCol) {
            prefix.push_back(&element);
        } else if (element.col >= joinCol) {
            const int suffixLimit =
                rung.outputBranchForkCol > 0 ? rung.outputBranchForkCol : coil.col;
            if (element.col < suffixLimit) {
                suffix.push_back(&element);
            }
        }
    }
    std::sort(prefix.begin(), prefix.end(), [](const LadderElement *a, const LadderElement *b) {
        return a->col < b->col;
    });
    std::sort(suffix.begin(), suffix.end(), [](const LadderElement *a, const LadderElement *b) {
        return a->col < b->col;
    });

    QStringList spanPaths;
    const QVector<LadderInputBranchRegion> inputRegions = LadderBranchLogic::inputBranchRegions(rung);
    if (!inputRegions.isEmpty()) {
        LadderRung regionRung = rung;
        regionRung.inputBranchRegions = inputRegions;
        const QVector<const LadderInputBranchRegion *> roots =
            LadderBranchLogic::rootInputRegions(regionRung);
        QString spanExpr = boolLiteral(true);
        if (!roots.isEmpty()) {
            spanExpr = buildInputRegionSpanExpr(regionRung, *roots.first(), andElements);
        }
        QStringList parts;
        const QString prefixExpr = andElements(prefix);
        if (prefixExpr != boolLiteral(true)) {
            parts.push_back(prefixExpr);
        }
        parts.push_back(QStringLiteral("(%1)").arg(spanExpr));
        const QString suffixExpr = andElements(suffix);
        if (suffixExpr != boolLiteral(true)) {
            parts.push_back(suffixExpr);
        }
        return parts.join(QStringLiteral(" AND "));
    }

    int maxRow = rung.branchRowsBelow;
    for (const LadderElement &element : rung.elements) {
        maxRow = qMax(maxRow, element.row);
    }
    for (int row = 0; row <= maxRow; ++row) {
        QVector<const LadderElement *> spanElements;
        for (const LadderElement &element : rung.elements) {
            if (element.row == row && element.kind != LadderElementKind::Coil &&
                element.col >= forkCol && element.col < joinCol) {
                spanElements.push_back(&element);
            }
        }
        std::sort(spanElements.begin(), spanElements.end(), [](const LadderElement *a, const LadderElement *b) {
            return a->col < b->col;
        });
        if (!spanElements.isEmpty()) {
            spanPaths.push_back(andElements(spanElements));
        }
    }

    QString spanExpr = spanPaths.isEmpty() ? boolLiteral(true) : spanPaths.join(QStringLiteral(" OR "));
    QStringList parts;
    const QString prefixExpr = andElements(prefix);
    if (prefixExpr != boolLiteral(true)) {
        parts.push_back(prefixExpr);
    }
    parts.push_back(QStringLiteral("(%1)").arg(spanExpr));
    const QString suffixExpr = andElements(suffix);
    if (suffixExpr != boolLiteral(true)) {
        parts.push_back(suffixExpr);
    }
    return parts.join(QStringLiteral(" AND "));
}

QString buildRungCondition(const LadderRung &rung, const LadderElement &coil)
{
    int forkCol = 0;
    int joinCol = 0;
    if (rung.branchSpan(&forkCol, &joinCol)) {
        return buildForkJoinCondition(rung, coil, forkCol, joinCol);
    }

    QMap<int, const LadderElement *> byId;
    for (const LadderElement &element : rung.elements) {
        byId.insert(element.localId, &element);
    }

    const QVector<int> incoming = incomingNodes(rung, coil.localId);
    if (incoming.isEmpty()) {
        QStringList series;
        for (const LadderElement &element : rung.elements) {
            if (element.kind == LadderElementKind::Contact) {
                series.push_back(contactExpr(element));
            } else if (element.kind == LadderElementKind::FunctionBlock) {
                series.push_back(QStringLiteral("%1.Q").arg(sanitizeInstanceName(element.fbTypeName, element.localId)));
            }
        }
        if (series.isEmpty()) {
            return boolLiteral(true);
        }
        return series.join(QStringLiteral(" AND "));
    }

    QStringList branchExprs;
    for (int sourceId : incoming) {
        const LadderElement *source = byId.value(sourceId, nullptr);
        if (source != nullptr && source->kind == LadderElementKind::FunctionBlock) {
            branchExprs.push_back(QStringLiteral("%1.Q")
                                      .arg(sanitizeInstanceName(source->fbTypeName, source->localId)));
        } else {
            QSet<int> visiting;
            branchExprs.push_back(buildPathExpr(rung, sourceId, byId, visiting));
        }
    }
    if (branchExprs.size() == 1) {
        return branchExprs.first();
    }
    return QStringLiteral("(%1)").arg(branchExprs.join(QStringLiteral(" OR ")));
}

QString assignmentForCoil(const LadderElement &coil, const QString &condition)
{
    const QString target = varRef(coil.variable);
    switch (coil.coilKind) {
    case LadderCoilKind::Set:
        return QStringLiteral("IF %1 THEN %2 := TRUE; END_IF;").arg(condition, target);
    case LadderCoilKind::Reset:
        return QStringLiteral("IF %1 THEN %2 := FALSE; END_IF;").arg(condition, target);
    case LadderCoilKind::Normal:
    default:
        return QStringLiteral("%1 := %2;").arg(target, condition);
    }
}

} // namespace

LadderStExporter::Result LadderStExporter::exportProgram(const LadderProgram &program)
{
    Result result;
    const QVector<LadderValidator::Issue> issues = LadderValidator::validate(program);
    for (const LadderValidator::Issue &issue : issues) {
        if (issue.severity == LadderValidator::Issue::Severity::Error) {
            result.diagnostics.push_back(issue.message);
        }
    }
    if (!result.diagnostics.isEmpty()) {
        result.ok = false;
        return result;
    }

    QStringList fbDecls;
    QSet<QString> fbInstances;
    QStringList bodyLines;

    for (const LadderRung &rung : program.rungs) {
        for (const LadderElement &element : rung.elements) {
            if (element.kind != LadderElementKind::FunctionBlock) {
                continue;
            }
            const QString instance = sanitizeInstanceName(element.fbTypeName, element.localId);
            if (fbInstances.contains(instance)) {
                continue;
            }
            fbInstances.insert(instance);
            fbDecls.push_back(QStringLiteral("    %1 : %2;").arg(instance, element.fbTypeName));
        }
    }

    for (int rungIndex = 0; rungIndex < program.rungs.size(); ++rungIndex) {
        const LadderRung &rung = program.rungs[rungIndex];
        QVector<const LadderElement *> coils;
        for (const LadderElement &element : rung.elements) {
            if (element.kind == LadderElementKind::Coil) {
                coils.push_back(&element);
            }
        }
        if (coils.isEmpty()) {
            result.diagnostics.push_back(QStringLiteral("Rung %1 skipped: no coil.").arg(rungIndex + 1));
            continue;
        }

        std::sort(coils.begin(), coils.end(), [](const LadderElement *a, const LadderElement *b) {
            if (a->row != b->row) {
                return a->row < b->row;
            }
            return a->col < b->col;
        });

        for (const LadderElement &element : rung.elements) {
            if (element.kind != LadderElementKind::FunctionBlock) {
                continue;
            }
            const QString instance = sanitizeInstanceName(element.fbTypeName, element.localId);
            QStringList assignments;
            QMap<int, const LadderElement *> byId;
            for (const LadderElement &rungElement : rung.elements) {
                byId.insert(rungElement.localId, &rungElement);
            }

            for (const LadderPinBinding &binding : element.pinBindings) {
                if (!LadderModel::isConfigurableInputPin(element.fbTypeName, binding.formalName)) {
                    continue;
                }
                const QString assignment = pinAssignment(binding);
                if (!assignment.isEmpty()) {
                    assignments.push_back(assignment);
                }
            }

            for (const LadderFbDescriptor &desc : LadderModel::functionBlockCatalog()) {
                if (desc.typeName.compare(element.fbTypeName, Qt::CaseInsensitive) != 0) {
                    continue;
                }
                for (const QString &pinName : desc.inputPins) {
                    if (!LadderModel::isRungWiredInputPin(pinName)) {
                        continue;
                    }
                    if (assignments.join(QString()).contains(QStringLiteral("%1 :=").arg(pinName))) {
                        continue;
                    }
                    QSet<int> visiting;
                    const QString feed = buildFeedExpr(rung, element.localId, byId, visiting);
                    if (!feed.isEmpty() && feed != boolLiteral(true)) {
                        assignments.prepend(QStringLiteral("%1 := %2").arg(pinName, feed));
                    }
                }
                break;
            }
            if (!assignments.isEmpty()) {
                bodyLines.push_back(QStringLiteral("    %1(%2);")
                                        .arg(instance, assignments.join(QStringLiteral(", "))));
            } else {
                bodyLines.push_back(QStringLiteral("    %1();").arg(instance));
            }
        }

        for (const LadderElement *coil : coils) {
            const QString condition = buildRungCondition(rung, *coil);
            bodyLines.push_back(QStringLiteral("    %1").arg(assignmentForCoil(*coil, condition)));
        }
    }

    QStringList varDecls;
    for (const LadderVariable &var : program.variables) {
        switch (var.type) {
        case LadderVarType::Int:
            varDecls.push_back(QStringLiteral("    %1 : INT;").arg(var.name));
            break;
        case LadderVarType::Time:
            varDecls.push_back(QStringLiteral("    %1 : TIME;").arg(var.name));
            break;
        case LadderVarType::Bool:
        default:
            if (!var.ioAddress.isEmpty()) {
                varDecls.push_back(QStringLiteral("    %1 AT %2 : BOOL;").arg(var.name, var.ioAddress));
            } else {
                varDecls.push_back(QStringLiteral("    %1 : BOOL;").arg(var.name));
            }
            break;
        }
    }

    QStringList lines;
    lines.push_back(QStringLiteral("PROGRAM %1").arg(program.name));
    lines.push_back(QStringLiteral("VAR"));
    lines.append(varDecls);
    lines.append(fbDecls);
    lines.push_back(QStringLiteral("END_VAR"));
    lines.append(bodyLines);
    lines.push_back(QStringLiteral("END_PROGRAM"));

    result.source = lines.join('\n');
    result.ok = true;
    return result;
}
