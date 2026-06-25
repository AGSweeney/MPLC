/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderModel.h"

#include <QWidget>

class QTableWidget;

class LadderVariableTable : public QWidget
{
    Q_OBJECT

public:
    explicit LadderVariableTable(QWidget *parent = nullptr);

    void setProgram(LadderProgram *program);
    void refresh();

    void addBoardIoVariables();

signals:
    void programModified();

private:
    void addVariableRow();
    void removeSelectedRows();
    void syncFromTable();
    void setRowFromVariable(int row, const LadderVariable &var);

    LadderProgram *m_program = nullptr;
    QTableWidget *m_table = nullptr;
    bool m_refreshing = false;
};
