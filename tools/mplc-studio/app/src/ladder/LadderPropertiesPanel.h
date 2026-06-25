/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderModel.h"

#include <QPointer>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class LadderElementItem;

class LadderPropertiesPanel : public QWidget
{
    Q_OBJECT

public:
    explicit LadderPropertiesPanel(QWidget *parent = nullptr);

    void setProgram(LadderProgram *program);
    void setSelectedItem(LadderElementItem *item);
    int selectedLocalId() const { return m_selectedLocalId; }

signals:
    void propertiesChanged();
    void editRequested();

private:
    void refreshFields();
    void applyVariable();
    void applyCoilKind(int index);
    void applyNegated(bool negated);
    LadderElement *elementForEdit() const;

    LadderProgram *m_program = nullptr;
    QPointer<LadderElementItem> m_item;
    int m_selectedLocalId = 0;
    QLabel *m_titleLabel = nullptr;
    QComboBox *m_variableCombo = nullptr;
    QCheckBox *m_negatedCheck = nullptr;
    QComboBox *m_coilKindCombo = nullptr;
    QLabel *m_fbTypeLabel = nullptr;
    QPushButton *m_editButton = nullptr;
    bool m_updating = false;
};
