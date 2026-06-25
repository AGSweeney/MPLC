/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderValidator.h"

#include <QSet>

QVector<LadderValidator::Issue> LadderValidator::validate(const LadderProgram &program)
{
    QVector<Issue> issues;

    QSet<QString> knownVars;
    for (const LadderVariable &var : program.variables) {
        knownVars.insert(var.name.toLower());
    }

    for (int rungIndex = 0; rungIndex < program.rungs.size(); ++rungIndex) {
        const LadderRung &rung = program.rungs[rungIndex];
        int coilCount = 0;

        for (const LadderElement &element : rung.elements) {
            if (element.kind == LadderElementKind::Coil) {
                ++coilCount;
            }

            if (element.kind == LadderElementKind::Contact || element.kind == LadderElementKind::Coil) {
                if (element.variable.isEmpty()) {
                    Issue issue;
                    issue.severity = Issue::Severity::Error;
                    issue.rungIndex = rungIndex;
                    issue.localId = element.localId;
                    issue.message = QStringLiteral("Element %1 is missing a variable name.").arg(element.localId);
                    issues.push_back(issue);
                } else if (!knownVars.contains(element.variable.toLower())) {
                    Issue issue;
                    issue.severity = Issue::Severity::Warning;
                    issue.rungIndex = rungIndex;
                    issue.localId = element.localId;
                    issue.message = QStringLiteral("Variable '%1' is not declared.").arg(element.variable);
                    issues.push_back(issue);
                }
            }

            if (element.kind == LadderElementKind::FunctionBlock && element.fbTypeName.isEmpty()) {
                Issue issue;
                issue.severity = Issue::Severity::Error;
                issue.rungIndex = rungIndex;
                issue.localId = element.localId;
                issue.message = QStringLiteral("Function block is missing a type name.");
                issues.push_back(issue);
            }
        }

        if (coilCount == 0) {
            Issue issue;
            issue.severity = Issue::Severity::Warning;
            issue.rungIndex = rungIndex;
            issue.message = QStringLiteral("Rung %1 has no output coil.").arg(rungIndex + 1);
            issues.push_back(issue);
        } else if (coilCount > 1 && !rung.hasOutputBranch()) {
            Issue issue;
            issue.severity = Issue::Severity::Warning;
            issue.rungIndex = rungIndex;
            issue.message = QStringLiteral("Rung %1 has multiple output coils without an output branch.")
                                .arg(rungIndex + 1);
            issues.push_back(issue);
        }

        if (rung.elements.size() > 1 && rung.connections.isEmpty()) {
            Issue issue;
            issue.severity = Issue::Severity::Error;
            issue.rungIndex = rungIndex;
            issue.message = QStringLiteral("Rung %1 has no connections.").arg(rungIndex + 1);
            issues.push_back(issue);
        }
    }

    return issues;
}
