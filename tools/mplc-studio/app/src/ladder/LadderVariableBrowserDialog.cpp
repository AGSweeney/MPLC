/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderVariableBrowserDialog.h"
#include "LadderVariableTable.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

LadderVariableBrowserDialog::LadderVariableBrowserDialog(LadderProgram *program, QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("LadderVariableBrowserDialog"));
    setWindowTitle(QStringLiteral("Variable Browser — MPLC Studio"));
    setModal(true);
    resize(720, 480);
    setMinimumSize(560, 360);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_table = new LadderVariableTable(this);
    m_table->setMinimumHeight(320);
    m_table->setProgram(program);
    layout->addWidget(m_table, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    layout->addWidget(buttons);

    connect(m_table, &LadderVariableTable::programModified, this, &LadderVariableBrowserDialog::programModified);
}
