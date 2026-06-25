/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderPropertiesPanel.h"
#include "LadderElementItem.h"
#include "LadderVariableUi.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

LadderPropertiesPanel::LadderPropertiesPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("LadderPropertiesPanel");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *headerBar = new QWidget(this);
    headerBar->setObjectName("LadderVariableHeader");
    auto *headerLayout = new QVBoxLayout(headerBar);
    headerLayout->setContentsMargins(10, 8, 10, 8);
    m_titleLabel = new QLabel(QStringLiteral("Element Properties"), headerBar);
    m_titleLabel->setObjectName("LadderBarTitle");
    headerLayout->addWidget(m_titleLabel);
    layout->addWidget(headerBar);

    auto *body = new QWidget(this);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(8, 8, 8, 8);
    bodyLayout->setSpacing(6);

    auto *formHost = new QWidget(body);
    auto *form = new QFormLayout(formHost);
    form->setContentsMargins(0, 0, 0, 0);
    form->setSpacing(6);

    m_variableCombo = new QComboBox(formHost);
    m_variableCombo->setEditable(true);
    m_variableCombo->setObjectName("LadderPropertyCombo");
    form->addRow(QStringLiteral("Variable"), m_variableCombo);

    m_negatedCheck = new QCheckBox(QStringLiteral("Normally closed"), formHost);
    form->addRow(QString(), m_negatedCheck);

    m_coilKindCombo = new QComboBox(formHost);
    m_coilKindCombo->addItem(QStringLiteral("Output"), static_cast<int>(LadderCoilKind::Normal));
    m_coilKindCombo->addItem(QStringLiteral("Set (S)"), static_cast<int>(LadderCoilKind::Set));
    m_coilKindCombo->addItem(QStringLiteral("Reset (R)"), static_cast<int>(LadderCoilKind::Reset));
    m_coilKindCombo->setObjectName("LadderPropertyCombo");
    form->addRow(QStringLiteral("Coil type"), m_coilKindCombo);

    m_fbTypeLabel = new QLabel(QStringLiteral("-"), formHost);
    form->addRow(QStringLiteral("Block"), m_fbTypeLabel);

    m_editButton = new QPushButton(QStringLiteral("Edit…"), formHost);
    m_editButton->setObjectName("LadderBarButton");
    m_editButton->setToolTip(QStringLiteral("Open variable chooser or function block editor"));
    form->addRow(QString(), m_editButton);

    bodyLayout->addWidget(formHost);
    bodyLayout->addStretch(1);
    layout->addWidget(body, 1);

    connect(m_variableCombo, &QComboBox::currentTextChanged, this, [this](const QString &) {
        if (!m_updating) {
            applyVariable();
        }
    });
    connect(m_negatedCheck, &QCheckBox::toggled, this, &LadderPropertiesPanel::applyNegated);
    connect(m_coilKindCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &LadderPropertiesPanel::applyCoilKind);
    connect(m_editButton, &QPushButton::clicked, this, &LadderPropertiesPanel::editRequested);

    setSelectedItem(nullptr);
}

void LadderPropertiesPanel::setProgram(LadderProgram *program)
{
    m_program = program;
    refreshFields();
}

void LadderPropertiesPanel::setSelectedItem(LadderElementItem *item)
{
    m_item = item;
    m_selectedLocalId = item != nullptr ? item->localId() : 0;
    refreshFields();
}

LadderElement *LadderPropertiesPanel::elementForEdit() const
{
    if (m_program == nullptr || m_selectedLocalId <= 0) {
        return nullptr;
    }
    return m_program->elementById(m_selectedLocalId);
}

void LadderPropertiesPanel::refreshFields()
{
    m_updating = true;

    const QSignalBlocker variableBlocker(m_variableCombo);
    const QSignalBlocker negatedBlocker(m_negatedCheck);
    const QSignalBlocker coilKindBlocker(m_coilKindCombo);

    m_variableCombo->clear();
    const LadderElement *selectedElement = elementForEdit();
    if (m_program != nullptr && selectedElement != nullptr &&
        selectedElement->kind != LadderElementKind::FunctionBlock) {
        LadderVariableUi::populateVariableChoices(m_variableCombo, m_program, selectedElement->kind);
    }

    if (m_selectedLocalId <= 0 || selectedElement == nullptr) {
        m_titleLabel->setText(QStringLiteral("Element Properties"));
        m_variableCombo->setEnabled(false);
        m_negatedCheck->setVisible(false);
        m_coilKindCombo->setVisible(false);
        m_fbTypeLabel->setVisible(false);
        m_editButton->setVisible(false);
        m_updating = false;
        return;
    }

    LadderElement *element = elementForEdit();
    if (element == nullptr) {
        m_titleLabel->setText(QStringLiteral("Element Properties"));
        m_variableCombo->setEnabled(false);
        m_negatedCheck->setVisible(false);
        m_coilKindCombo->setVisible(false);
        m_fbTypeLabel->setVisible(false);
        m_editButton->setVisible(false);
        m_updating = false;
        return;
    }

    m_variableCombo->setEnabled(element->kind != LadderElementKind::FunctionBlock);
    m_negatedCheck->setVisible(element->kind == LadderElementKind::Contact);
    m_coilKindCombo->setVisible(element->kind == LadderElementKind::Coil);
    m_fbTypeLabel->setVisible(element->kind == LadderElementKind::FunctionBlock);
    m_editButton->setVisible(true);
    m_editButton->setText(element->kind == LadderElementKind::FunctionBlock
                              ? QStringLiteral("Edit Block…")
                              : QStringLiteral("Choose Variable…"));

    switch (element->kind) {
    case LadderElementKind::Contact:
        m_titleLabel->setText(QStringLiteral("Contact"));
        break;
    case LadderElementKind::Coil:
        m_titleLabel->setText(QStringLiteral("Coil"));
        break;
    case LadderElementKind::FunctionBlock:
        m_titleLabel->setText(QStringLiteral("Function Block"));
        break;
    }

    if (element->kind != LadderElementKind::FunctionBlock) {
        LadderVariableUi::selectVariable(m_variableCombo, element->variable);
    }

    m_negatedCheck->setChecked(element->negated);

    int coilIndex = 0;
    switch (element->coilKind) {
    case LadderCoilKind::Set:
        coilIndex = 1;
        break;
    case LadderCoilKind::Reset:
        coilIndex = 2;
        break;
    case LadderCoilKind::Normal:
    default:
        coilIndex = 0;
        break;
    }
    m_coilKindCombo->setCurrentIndex(coilIndex);
    m_fbTypeLabel->setText(element->fbTypeName);

    m_updating = false;
}

void LadderPropertiesPanel::applyVariable()
{
    if (m_updating || m_program == nullptr || m_selectedLocalId <= 0) {
        return;
    }

    LadderElement *element = elementForEdit();
    if (element == nullptr || element->kind == LadderElementKind::FunctionBlock) {
        return;
    }

    const QString name = LadderVariableUi::selectedVariableName(m_variableCombo);
    element->variable = name;
    if (!name.isEmpty()) {
        m_program->addVariableIfMissing(name);
    }
    if (m_item != nullptr) {
        m_item->update();
    }
    emit propertiesChanged();
}

void LadderPropertiesPanel::applyCoilKind(int index)
{
    if (m_updating || m_selectedLocalId <= 0) {
        return;
    }

    LadderElement *element = elementForEdit();
    if (element == nullptr || element->kind != LadderElementKind::Coil) {
        return;
    }

    element->coilKind = static_cast<LadderCoilKind>(m_coilKindCombo->itemData(index).toInt());
    if (m_item != nullptr) {
        m_item->update();
    }
    emit propertiesChanged();
}

void LadderPropertiesPanel::applyNegated(bool negated)
{
    if (m_updating || m_selectedLocalId <= 0) {
        return;
    }

    LadderElement *element = elementForEdit();
    if (element == nullptr || element->kind != LadderElementKind::Contact) {
        return;
    }

    element->negated = negated;
    if (m_item != nullptr) {
        m_item->update();
    }
    emit propertiesChanged();
}
