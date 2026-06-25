/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QDialog>

class LadderProgram;
class LadderVariableTable;

class LadderVariableBrowserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LadderVariableBrowserDialog(LadderProgram *program, QWidget *parent = nullptr);

signals:
    void programModified();

private:
    LadderVariableTable *m_table = nullptr;
};
