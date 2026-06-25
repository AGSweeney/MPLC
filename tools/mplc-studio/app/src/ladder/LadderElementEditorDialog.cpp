/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderElementEditorDialog.h"
#include "LadderModel.h"
#include "LadderVariableUi.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QObject>

namespace {

const LadderFbDescriptor *descriptorForType(const QString &typeName)
{
    for (const LadderFbDescriptor &desc : LadderModel::functionBlockCatalog()) {
        if (desc.typeName.compare(typeName, Qt::CaseInsensitive) == 0) {
            return &desc;
        }
    }
    return nullptr;
}

LadderPinBinding *findPinBinding(LadderElement *element, const QString &formalName)
{
    for (LadderPinBinding &binding : element->pinBindings) {
        if (binding.formalName.compare(formalName, Qt::CaseInsensitive) == 0) {
            return &binding;
        }
    }
    return nullptr;
}

LadderPinBinding *ensurePinBinding(LadderElement *element, const QString &formalName)
{
    if (LadderPinBinding *binding = findPinBinding(element, formalName)) {
        return binding;
    }
    LadderPinBinding newBinding;
    newBinding.formalName = formalName;
    element->pinBindings.push_back(newBinding);
    return &element->pinBindings.last();
}

void clearRungWiredBindings(LadderElement *element)
{
    QVector<LadderPinBinding> kept;
    for (const LadderPinBinding &binding : element->pinBindings) {
        if (!LadderModel::isRungWiredInputPin(binding.formalName)) {
            kept.push_back(binding);
        }
    }
    element->pinBindings = kept;
}

bool editContactOrCoil(LadderProgram *program, LadderElement *element, QWidget *parent)
{
    if (program == nullptr || element == nullptr) {
        return false;
    }

    const int localId = element->localId;
    const LadderElementKind elementKind = element->kind;

    QDialog dialog(parent);
    dialog.setModal(true);
    dialog.setWindowTitle(elementKind == LadderElementKind::Contact
                              ? QStringLiteral("Edit Contact")
                              : QStringLiteral("Edit Coil"));

    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto *variableCombo = new QComboBox(&dialog);
    variableCombo->setEditable(true);
    variableCombo->setObjectName(QStringLiteral("LadderPropertyCombo"));
    LadderVariableUi::populateVariableChoices(variableCombo, program, elementKind);
    LadderVariableUi::selectVariable(variableCombo, element->variable);
    form->addRow(QStringLiteral("Variable"), variableCombo);

    auto *negatedCheck = new QCheckBox(QStringLiteral("Normally closed"), &dialog);
    negatedCheck->setChecked(element->negated);
    negatedCheck->setVisible(elementKind == LadderElementKind::Contact);
    if (elementKind == LadderElementKind::Contact) {
        form->addRow(QString(), negatedCheck);
    }

    auto *coilKindCombo = new QComboBox(&dialog);
    coilKindCombo->addItem(QStringLiteral("Output"), static_cast<int>(LadderCoilKind::Normal));
    coilKindCombo->addItem(QStringLiteral("Set (S)"), static_cast<int>(LadderCoilKind::Set));
    coilKindCombo->addItem(QStringLiteral("Reset (R)"), static_cast<int>(LadderCoilKind::Reset));
    coilKindCombo->setVisible(elementKind == LadderElementKind::Coil);
    if (elementKind == LadderElementKind::Coil) {
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
        coilKindCombo->setCurrentIndex(coilIndex);
        form->addRow(QStringLiteral("Coil type"), coilKindCombo);
    }

    layout->addLayout(form);

    QString chosenVariable;
    bool chosenNegated = false;
    LadderCoilKind chosenCoilKind = LadderCoilKind::Normal;

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        chosenVariable = LadderVariableUi::selectedVariableName(variableCombo);
        if (elementKind == LadderElementKind::Contact) {
            chosenNegated = negatedCheck->isChecked();
        } else if (elementKind == LadderElementKind::Coil) {
            chosenCoilKind = static_cast<LadderCoilKind>(coilKindCombo->currentData().toInt());
        }
        dialog.accept();
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    element = program->elementById(localId);
    if (element == nullptr) {
        return false;
    }

    element->variable = chosenVariable;
    if (!chosenVariable.isEmpty()) {
        program->addVariableIfMissing(chosenVariable);
    }

    if (elementKind == LadderElementKind::Contact) {
        element->negated = chosenNegated;
    } else if (elementKind == LadderElementKind::Coil) {
        element->coilKind = chosenCoilKind;
    }

    return true;
}

bool editTimerBlock(LadderProgram *program, LadderElement *element, const LadderFbDescriptor *descriptor, QWidget *parent)
{
    if (program == nullptr || element == nullptr || descriptor == nullptr) {
        return false;
    }

    const int localId = element->localId;

    QDialog dialog(parent);
    dialog.setModal(true);
    dialog.setWindowTitle(QStringLiteral("Edit %1").arg(element->fbTypeName));

    auto *layout = new QVBoxLayout(&dialog);

    auto *note = new QLabel(QStringLiteral("The timer input comes from the rung wire to the left of the block."),
                            &dialog);
    note->setWordWrap(true);
    layout->addWidget(note);

    const LadderPinBinding *existingPt = findPinBinding(element, QStringLiteral("PT"));
    const QString defaultTime = descriptor->defaultLiterals.value(QStringLiteral("PT"), QStringLiteral("T#1s"));
    const bool useVariable = existingPt != nullptr && !existingPt->variable.isEmpty();

    auto *constantRadio = new QRadioButton(QStringLiteral("Time constant"), &dialog);
    auto *variableRadio = new QRadioButton(QStringLiteral("Variable"), &dialog);
    constantRadio->setChecked(!useVariable);
    variableRadio->setChecked(useVariable);

    auto *timeEdit = new QLineEdit(&dialog);
    timeEdit->setPlaceholderText(QStringLiteral("e.g. T#3s, T#500ms, 1000"));
    if (existingPt != nullptr && !useVariable && !existingPt->literal.isEmpty()) {
        timeEdit->setText(existingPt->literal);
    } else {
        timeEdit->setText(defaultTime);
    }

    auto *variableCombo = new QComboBox(&dialog);
    variableCombo->setObjectName(QStringLiteral("LadderPropertyCombo"));
    LadderVariableUi::populateTimeVariableChoices(variableCombo, program);
    if (useVariable && existingPt != nullptr) {
        LadderVariableUi::selectVariable(variableCombo, existingPt->variable);
    }

    auto *form = new QFormLayout();
    form->addRow(constantRadio, timeEdit);
    form->addRow(variableRadio, variableCombo);
    layout->addLayout(form);

    variableCombo->setEnabled(useVariable);
    timeEdit->setEnabled(!useVariable);
    QObject::connect(constantRadio, &QRadioButton::toggled, &dialog, [timeEdit, variableCombo](bool checked) {
        timeEdit->setEnabled(checked);
        variableCombo->setEnabled(!checked);
    });

    if (!descriptor->outputPins.isEmpty()) {
        auto *outputsGroup = new QGroupBox(QStringLiteral("Output"), &dialog);
        auto *outputsForm = new QFormLayout(outputsGroup);
        outputsForm->addRow(descriptor->outputPins.first(),
                            new QLabel(QStringLiteral("Connected by the rung wire to the right"), outputsGroup));
        layout->addWidget(outputsGroup);
    }

    bool useConstant = true;
    QString chosenLiteral;
    QString chosenVariable;

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        useConstant = constantRadio->isChecked();
        chosenLiteral = timeEdit->text().trimmed();
        chosenVariable = LadderVariableUi::selectedVariableName(variableCombo);
        dialog.accept();
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    element = program->elementById(localId);
    if (element == nullptr) {
        return false;
    }

    clearRungWiredBindings(element);
    LadderPinBinding *ptBinding = ensurePinBinding(element, QStringLiteral("PT"));
    if (useConstant) {
        ptBinding->variable.clear();
        ptBinding->literal = chosenLiteral.isEmpty() ? defaultTime : chosenLiteral;
    } else {
        ptBinding->literal.clear();
        ptBinding->variable = chosenVariable;
        if (!ptBinding->variable.isEmpty()) {
            program->addVariableIfMissing(ptBinding->variable, LadderVarType::Time);
        }
    }

    return true;
}

bool editFunctionBlock(LadderProgram *program, LadderElement *element, QWidget *parent)
{
    if (program == nullptr || element == nullptr) {
        return false;
    }

    const int localId = element->localId;
    const LadderFbDescriptor *descriptor = descriptorForType(element->fbTypeName);
    if (descriptor == nullptr) {
        return false;
    }

    if (LadderModel::isTimerFunctionBlock(element->fbTypeName)) {
        return editTimerBlock(program, element, descriptor, parent);
    }

    QDialog dialog(parent);
    dialog.setModal(true);
    dialog.setWindowTitle(QStringLiteral("Edit %1").arg(element->fbTypeName));

    auto *layout = new QVBoxLayout(&dialog);

    auto *inputsGroup = new QGroupBox(QStringLiteral("Inputs"), &dialog);
    auto *inputsForm = new QFormLayout(inputsGroup);
    inputsForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    struct PinRow {
        QString formalName;
        QComboBox *variableCombo = nullptr;
        QLineEdit *literalEdit = nullptr;
        QString chosenVariable;
        QString chosenLiteral;
    };
    QVector<PinRow> rows;

    for (const QString &pinName : descriptor->inputPins) {
        if (!LadderModel::isConfigurableInputPin(element->fbTypeName, pinName)) {
            continue;
        }

        PinRow row;
        row.formalName = pinName;

        row.variableCombo = new QComboBox(inputsGroup);
        row.variableCombo->setEditable(true);
        row.variableCombo->setObjectName(QStringLiteral("LadderPropertyCombo"));
        LadderVariableUi::populateVariableChoices(row.variableCombo, program, LadderElementKind::Contact);

        row.literalEdit = new QLineEdit(inputsGroup);
        row.literalEdit->setPlaceholderText(QStringLiteral("Literal value"));

        if (LadderPinBinding *binding = findPinBinding(element, pinName)) {
            LadderVariableUi::selectVariable(row.variableCombo, binding->variable);
            row.literalEdit->setText(binding->literal);
        } else if (descriptor->defaultLiterals.contains(pinName)) {
            row.literalEdit->setText(descriptor->defaultLiterals.value(pinName));
        }

        auto *pinRow = new QWidget(inputsGroup);
        auto *pinLayout = new QHBoxLayout(pinRow);
        pinLayout->setContentsMargins(0, 0, 0, 0);
        pinLayout->addWidget(row.variableCombo, 1);
        pinLayout->addWidget(new QLabel(QStringLiteral("or"), pinRow));
        pinLayout->addWidget(row.literalEdit, 1);

        inputsForm->addRow(pinName, pinRow);
        rows.push_back(row);
    }

    if (rows.isEmpty()) {
        inputsForm->addRow(new QLabel(QStringLiteral("Configure this block from the rung wiring."), inputsGroup));
    }

    layout->addWidget(inputsGroup);

    if (!descriptor->outputPins.isEmpty()) {
        auto *outputsGroup = new QGroupBox(QStringLiteral("Outputs"), &dialog);
        auto *outputsForm = new QFormLayout(outputsGroup);
        for (const QString &pinName : descriptor->outputPins) {
            outputsForm->addRow(pinName, new QLabel(QStringLiteral("(block output)"), outputsGroup));
        }
        layout->addWidget(outputsGroup);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        for (PinRow &row : rows) {
            row.chosenVariable = LadderVariableUi::selectedVariableName(row.variableCombo);
            row.chosenLiteral = row.literalEdit->text().trimmed();
        }
        dialog.accept();
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    element = program->elementById(localId);
    if (element == nullptr) {
        return false;
    }

    clearRungWiredBindings(element);

    for (const PinRow &row : rows) {
        LadderPinBinding *binding = ensurePinBinding(element, row.formalName);
        binding->variable = row.chosenVariable;
        binding->literal = row.chosenLiteral;

        if (!binding->variable.isEmpty()) {
            program->addVariableIfMissing(binding->variable);
        }
    }

    return true;
}

} // namespace

bool LadderElementEditorDialog::editElement(LadderProgram *program, LadderElement *element, QWidget *parent)
{
    if (program == nullptr || element == nullptr) {
        return false;
    }

    const int localId = element->localId;

    bool accepted = false;
    switch (element->kind) {
    case LadderElementKind::Contact:
    case LadderElementKind::Coil:
        accepted = editContactOrCoil(program, element, parent);
        break;
    case LadderElementKind::FunctionBlock:
        accepted = editFunctionBlock(program, element, parent);
        break;
    default:
        return false;
    }

    return accepted && program->elementById(localId) != nullptr;
}
