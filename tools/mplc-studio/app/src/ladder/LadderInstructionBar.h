/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QWidget>

class LadderInstructionBar : public QWidget
{
    Q_OBJECT

public:
    explicit LadderInstructionBar(QWidget *parent = nullptr);

signals:
    void addContact(bool negated);
    void addCoil();
    void addSetCoil();
    void addResetCoil();
    void addFunctionBlock(const QString &typeName);
    void addRung();
    void surroundWithBranch();
    void addParallelBranchLeg();
    void removeBranch();
    void deleteSelection();
    void deleteRung();
    void viewGeneratedSt();
    void browseVariables();

private:
    QWidget *makeButton(const QString &label, const QString &tooltip);
};
