/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderVariableUi.h"
#include "LadderIoCatalog.h"

#include <QComboBox>

namespace LadderVariableUi {

namespace {

void addVariableChoice(QComboBox *combo, const LadderVariable &variable)
{
    if (const LadderIoPoint *point = LadderIoCatalog::findByName(variable.name)) {
        combo->addItem(QStringLiteral("%1 (%2)").arg(variable.name, point->roleLabel), variable.name);
    } else {
        combo->addItem(variable.name, variable.name);
    }
}

bool isCatalogInputName(const QString &name)
{
    const LadderIoPoint *point = LadderIoCatalog::findByName(name);
    return point != nullptr && point->isInput;
}

bool isCatalogOutputName(const QString &name)
{
    const LadderIoPoint *point = LadderIoCatalog::findByName(name);
    return point != nullptr && !point->isInput;
}

} // namespace

void populateVariableChoices(QComboBox *combo, const LadderProgram *program, LadderElementKind kind)
{
    combo->clear();
    if (program == nullptr) {
        return;
    }

    QVector<const LadderVariable *> dips;
    QVector<const LadderVariable *> leds;
    QVector<const LadderVariable *> internal;

    for (const LadderVariable &variable : program->variables) {
        if (isCatalogInputName(variable.name)) {
            dips.push_back(&variable);
        } else if (isCatalogOutputName(variable.name)) {
            leds.push_back(&variable);
        } else {
            internal.push_back(&variable);
        }
    }

    const auto appendGroup = [&](const QString &title, const QVector<const LadderVariable *> &vars) {
        if (vars.isEmpty()) {
            return;
        }
        if (combo->count() > 0) {
            combo->insertSeparator(combo->count());
        }
        combo->addItem(title);
        combo->model()->setData(combo->model()->index(combo->count() - 1, 0), 0, Qt::UserRole - 1);
        for (const LadderVariable *variable : vars) {
            addVariableChoice(combo, *variable);
        }
    };

    if (kind == LadderElementKind::Contact) {
        appendGroup(QStringLiteral("DIP Switches"), dips);
        appendGroup(QStringLiteral("Internal"), internal);
        appendGroup(QStringLiteral("LEDs"), leds);
    } else if (kind == LadderElementKind::Coil) {
        appendGroup(QStringLiteral("LEDs"), leds);
        appendGroup(QStringLiteral("Internal"), internal);
        appendGroup(QStringLiteral("DIP Switches"), dips);
    } else {
        appendGroup(QStringLiteral("DIP Switches"), dips);
        appendGroup(QStringLiteral("LEDs"), leds);
        appendGroup(QStringLiteral("Internal"), internal);
    }
}

int comboIndexForVariable(QComboBox *combo, const QString &variableName)
{
    for (int i = 0; i < combo->count(); ++i) {
        if (combo->itemData(i).toString() == variableName) {
            return i;
        }
    }
    return -1;
}

QString selectedVariableName(QComboBox *combo)
{
    if (combo == nullptr) {
        return QString();
    }
    const QVariant data = combo->currentData();
    if (data.isValid() && !data.toString().isEmpty()) {
        return data.toString();
    }
    return combo->currentText().trimmed();
}

void selectVariable(QComboBox *combo, const QString &variableName)
{
    if (combo == nullptr) {
        return;
    }
    const int index = comboIndexForVariable(combo, variableName);
    if (index >= 0) {
        combo->setCurrentIndex(index);
    } else {
        combo->setEditText(variableName);
    }
}

void populateTimeVariableChoices(QComboBox *combo, const LadderProgram *program)
{
    combo->clear();
    if (program == nullptr) {
        return;
    }

    QVector<const LadderVariable *> timeVars;
    for (const LadderVariable &variable : program->variables) {
        if (LadderIoCatalog::findByName(variable.name) != nullptr) {
            continue;
        }
        if (variable.type == LadderVarType::Time || variable.type == LadderVarType::Int) {
            timeVars.push_back(&variable);
        }
    }

    if (timeVars.isEmpty()) {
        combo->addItem(QStringLiteral("(no TIME/INT variables)"));
        combo->model()->setData(combo->model()->index(0, 0), 0, Qt::UserRole - 1);
        return;
    }

    for (const LadderVariable *variable : timeVars) {
        addVariableChoice(combo, *variable);
    }
}

} // namespace LadderVariableUi
