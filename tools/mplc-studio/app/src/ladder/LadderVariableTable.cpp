/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderVariableTable.h"
#include "LadderIoCatalog.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSet>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <algorithm>

namespace {

QString typeToText(LadderVarType type)
{
    switch (type) {
    case LadderVarType::Int:
        return QStringLiteral("INT");
    case LadderVarType::Time:
        return QStringLiteral("TIME");
    case LadderVarType::Bool:
    default:
        return QStringLiteral("BOOL");
    }
}

LadderVarType typeFromText(const QString &typeText)
{
    const QString upper = typeText.trimmed().toUpper();
    if (upper == QStringLiteral("INT") || upper == QStringLiteral("DINT")) {
        return LadderVarType::Int;
    }
    if (upper == QStringLiteral("TIME")) {
        return LadderVarType::Time;
    }
    return LadderVarType::Bool;
}

bool isBoardIoVariable(const QString &name)
{
    return LadderIoCatalog::findByName(name) != nullptr;
}

QComboBox *makeTypeComboBox(QWidget *parent, LadderVarType type, bool editable)
{
    auto *combo = new QComboBox(parent);
    combo->setObjectName(QStringLiteral("LadderVarTypeCombo"));
    combo->addItem(QStringLiteral("BOOL"), static_cast<int>(LadderVarType::Bool));
    combo->addItem(QStringLiteral("INT"), static_cast<int>(LadderVarType::Int));
    combo->addItem(QStringLiteral("TIME"), static_cast<int>(LadderVarType::Time));

    const int index = combo->findData(static_cast<int>(type));
    combo->setCurrentIndex(index >= 0 ? index : 0);
    combo->setEnabled(editable);
    if (!editable) {
        combo->setToolTip(QStringLiteral("Board I/O variables are always BOOL"));
    }
    return combo;
}

LadderVarType typeFromCombo(const QComboBox *combo)
{
    if (combo == nullptr) {
        return LadderVarType::Bool;
    }
    return static_cast<LadderVarType>(combo->currentData().toInt());
}

} // namespace

LadderVariableTable::LadderVariableTable(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("LadderVariableTable");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *headerBar = new QWidget(this);
    headerBar->setObjectName("LadderVariableHeader");
    auto *headerLayout = new QHBoxLayout(headerBar);
    headerLayout->setContentsMargins(10, 6, 10, 6);
    headerLayout->setSpacing(8);

    auto *titleColumn = new QVBoxLayout();
    titleColumn->setContentsMargins(0, 0, 0, 0);
    titleColumn->setSpacing(0);
    auto *header = new QLabel(QStringLiteral("Global Variables"), headerBar);
    header->setObjectName("LadderBarTitle");
    auto *subtitle = new QLabel(QStringLiteral("Board I/O: Dip1–Dip8 (inputs), Led1–Led8 (outputs)"), headerBar);
    subtitle->setObjectName("LadderBarSubtitle");
    titleColumn->addWidget(header);
    titleColumn->addWidget(subtitle);
    headerLayout->addLayout(titleColumn);
    headerLayout->addStretch(1);

    auto *boardIoButton = new QPushButton(QStringLiteral("Add Board I/O"), headerBar);
    boardIoButton->setObjectName("LadderBarButton");
    boardIoButton->setToolTip(QStringLiteral("Add MOD-DEV-70 DIP and LED global variables (Dip1–Led8)"));
    auto *addButton = new QPushButton(QStringLiteral("+ Add"), headerBar);
    addButton->setObjectName("LadderBarButton");
    auto *removeButton = new QPushButton(QStringLiteral("Remove"), headerBar);
    removeButton->setObjectName("LadderBarButton");
    headerLayout->addWidget(boardIoButton);
    headerLayout->addWidget(addButton);
    headerLayout->addWidget(removeButton);
    layout->addWidget(headerBar);

    m_table = new QTableWidget(0, 3, this);
    m_table->setObjectName("LadderVariableGrid");
    m_table->setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("Type"), QStringLiteral("I/O")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setShowGrid(true);
    layout->addWidget(m_table, 1);

    connect(boardIoButton, &QPushButton::clicked, this, &LadderVariableTable::addBoardIoVariables);
    connect(addButton, &QPushButton::clicked, this, &LadderVariableTable::addVariableRow);
    connect(removeButton, &QPushButton::clicked, this, &LadderVariableTable::removeSelectedRows);
    connect(m_table, &QTableWidget::cellChanged, this, [this](int, int) {
        if (!m_refreshing) {
            syncFromTable();
        }
    });
}

void LadderVariableTable::setProgram(LadderProgram *program)
{
    m_program = program;
    refresh();
}

void LadderVariableTable::setRowFromVariable(int row, const LadderVariable &var)
{
    const bool boardIo = isBoardIoVariable(var.name);

    auto *nameItem = new QTableWidgetItem(var.name);
    if (boardIo) {
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setToolTip(QStringLiteral("Board I/O variable"));
    }
    m_table->setItem(row, 0, nameItem);

    auto *typeCombo = makeTypeComboBox(m_table, var.type, !boardIo);
    connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_refreshing) {
            syncFromTable();
        }
    });
    m_table->setCellWidget(row, 1, typeCombo);

    auto *ioItem = new QTableWidgetItem(boardIo ? LadderIoCatalog::ioDisplayLabel(var) : var.ioAddress);
    if (boardIo) {
        ioItem->setFlags(ioItem->flags() & ~Qt::ItemIsEditable);
        ioItem->setToolTip(var.ioAddress);
    } else {
        ioItem->setToolTip(QStringLiteral("Optional I/O address, e.g. %IX0.0"));
    }
    m_table->setItem(row, 2, ioItem);
}

void LadderVariableTable::refresh()
{
    if (m_table == nullptr) {
        return;
    }

    m_refreshing = true;
    m_table->blockSignals(true);
    m_table->setRowCount(0);
    if (m_program == nullptr) {
        m_table->blockSignals(false);
        m_refreshing = false;
        return;
    }

    for (const LadderVariable &var : m_program->variables) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        setRowFromVariable(row, var);
    }
    m_table->blockSignals(false);
    m_refreshing = false;
}

void LadderVariableTable::addBoardIoVariables()
{
    if (m_program == nullptr) {
        return;
    }
    m_program->ensureBoardIoVariables();
    refresh();
    emit programModified();
}

void LadderVariableTable::addVariableRow()
{
    LadderVariable var;
    var.name = QStringLiteral("Var%1").arg(m_table->rowCount() + 1);
    var.type = LadderVarType::Bool;

    m_refreshing = true;
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    setRowFromVariable(row, var);
    m_refreshing = false;

    syncFromTable();
}

void LadderVariableTable::removeSelectedRows()
{
    QSet<int> rowsToRemove;
    for (const QTableWidgetSelectionRange &range : m_table->selectedRanges()) {
        for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
            const QTableWidgetItem *nameItem = m_table->item(row, 0);
            if (nameItem != nullptr && isBoardIoVariable(nameItem->text())) {
                continue;
            }
            rowsToRemove.insert(row);
        }
    }

    QList<int> sortedRows = rowsToRemove.values();
    std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());
    for (int row : sortedRows) {
        m_table->removeRow(row);
    }

    if (!rowsToRemove.isEmpty()) {
        syncFromTable();
    }
}

void LadderVariableTable::syncFromTable()
{
    if (m_program == nullptr || m_table == nullptr || m_refreshing) {
        return;
    }

    m_program->variables.clear();
    for (int row = 0; row < m_table->rowCount(); ++row) {
        LadderVariable var;
        if (QTableWidgetItem *nameItem = m_table->item(row, 0)) {
            var.name = nameItem->text().trimmed();
        }
        if (const LadderIoPoint *point = LadderIoCatalog::findByName(var.name)) {
            var.type = point->type;
            var.ioAddress = point->ioAddress;
        } else {
            if (QComboBox *typeCombo = qobject_cast<QComboBox *>(m_table->cellWidget(row, 1))) {
                var.type = typeFromCombo(typeCombo);
            }
            if (QTableWidgetItem *addrItem = m_table->item(row, 2)) {
                var.ioAddress = addrItem->text().trimmed();
            }
        }
        if (!var.name.isEmpty()) {
            m_program->variables.push_back(var);
        }
    }
    emit programModified();
}
