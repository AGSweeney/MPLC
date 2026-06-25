/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderModel.h"
#include "LadderBranchLogic.h"
#include "LadderIoCatalog.h"
#include "LadderPlcopenSerializer.h"

#include <QMap>
#include <QSet>
#include <algorithm>
#include <functional>
#include <limits>

namespace {

int computeOutputBranchForkCol(const LadderRung &rung, int minCoilCol)
{
    int inputForkCol = 0;
    int inputJoinCol = 0;
    const bool hasInput = rung.hasInputBranch() && rung.branchSpan(&inputForkCol, &inputJoinCol);
    const int scanStartCol = hasInput ? inputJoinCol : 0;

    int sharedEndCol = 0;
    for (const LadderElement &element : rung.elements) {
        if (element.row != 0 || element.kind == LadderElementKind::Coil) {
            continue;
        }
        if (element.col < scanStartCol || element.col >= minCoilCol) {
            continue;
        }
        sharedEndCol = qMax(sharedEndCol, element.col);
    }

    return sharedEndCol > 0 ? sharedEndCol + 1 : minCoilCol;
}

} // namespace

int LadderProgram::allocateLocalId()
{
    return nextLocalId++;
}

LadderRung *LadderProgram::rungAt(int index)
{
    if (index < 0 || index >= rungs.size()) {
        return nullptr;
    }
    return &rungs[index];
}

const LadderRung *LadderProgram::rungAt(int index) const
{
    if (index < 0 || index >= rungs.size()) {
        return nullptr;
    }
    return &rungs[index];
}

LadderElement *LadderProgram::elementById(int localId)
{
    for (LadderRung &rung : rungs) {
        for (LadderElement &element : rung.elements) {
            if (element.localId == localId) {
                return &element;
            }
        }
    }
    return nullptr;
}

const LadderElement *LadderProgram::elementById(int localId) const
{
    for (const LadderRung &rung : rungs) {
        for (const LadderElement &element : rung.elements) {
            if (element.localId == localId) {
                return &element;
            }
        }
    }
    return nullptr;
}

void LadderProgram::ensureBoardIoVariables()
{
    LadderIoCatalog::ensureModDev70Variables(*this);
}

void LadderProgram::addVariableIfMissing(const QString &name, LadderVarType type)
{
    if (name.isEmpty()) {
        return;
    }
    for (const LadderVariable &var : variables) {
        if (var.name.compare(name, Qt::CaseInsensitive) == 0) {
            return;
        }
    }
    LadderVariable var;
    var.name = name;
    var.type = type;
    if (const LadderIoPoint *point = LadderIoCatalog::findByName(name)) {
        var.type = point->type;
        var.ioAddress = point->ioAddress;
    }
    variables.push_back(var);
}

LadderRung &LadderProgram::addRung()
{
    LadderRung rung;
    rung.localId = allocateLocalId();
    rungs.push_back(rung);
    return rungs.last();
}

void LadderProgram::removeRung(int index)
{
    if (index < 0 || index >= rungs.size()) {
        return;
    }
    rungs.removeAt(index);
}

LadderElement &LadderProgram::addContact(int rungIndex, int row, int col, const QString &variable, bool negated)
{
    LadderRung *rung = rungAt(rungIndex);
    if (rung == nullptr) {
        rung = &addRung();
        rungIndex = rungs.size() - 1;
    }

    LadderElement element;
    element.localId = allocateLocalId();
    element.kind = LadderElementKind::Contact;
    element.rungIndex = rungIndex;
    element.row = row;
    element.col = col;
    element.variable = variable;
    element.negated = negated;
    rung->elements.push_back(element);
    addVariableIfMissing(variable);
    return rung->elements.last();
}

LadderElement &LadderProgram::addCoil(int rungIndex, int row, int col, const QString &variable, LadderCoilKind kind)
{
    LadderRung *rung = rungAt(rungIndex);
    if (rung == nullptr) {
        rung = &addRung();
        rungIndex = rungs.size() - 1;
    }

    LadderElement element;
    element.localId = allocateLocalId();
    element.kind = LadderElementKind::Coil;
    element.rungIndex = rungIndex;
    element.row = row;
    element.col = col;
    element.variable = variable;
    element.coilKind = kind;
    rung->elements.push_back(element);
    addVariableIfMissing(variable);
    return rung->elements.last();
}

LadderElement &LadderProgram::addFunctionBlock(int rungIndex, int row, int col, const QString &typeName)
{
    LadderRung *rung = rungAt(rungIndex);
    if (rung == nullptr) {
        rung = &addRung();
        rungIndex = rungs.size() - 1;
    }

    LadderElement element;
    element.localId = allocateLocalId();
    element.kind = LadderElementKind::FunctionBlock;
    element.rungIndex = rungIndex;
    element.row = row;
    element.col = col;
    element.fbTypeName = typeName;

    for (const LadderFbDescriptor &desc : LadderModel::functionBlockCatalog()) {
        if (desc.typeName.compare(typeName, Qt::CaseInsensitive) == 0) {
            for (const QString &pin : desc.inputPins) {
                if (!LadderModel::isConfigurableInputPin(typeName, pin)) {
                    continue;
                }
                LadderPinBinding binding;
                binding.formalName = pin;
                binding.literal = desc.defaultLiterals.value(pin);
                element.pinBindings.push_back(binding);
            }
            break;
        }
    }

    rung->elements.push_back(element);
    return rung->elements.last();
}

void LadderProgram::connectElements(int fromId, int toId, const QString &fromPin, const QString &toPin)
{
    const LadderElement *from = elementById(fromId);
    const LadderElement *to = elementById(toId);
    if (from == nullptr || to == nullptr || from->rungIndex != to->rungIndex) {
        return;
    }

    LadderRung *rung = rungAt(from->rungIndex);
    if (rung == nullptr) {
        return;
    }

    for (const LadderConnection &existing : rung->connections) {
        if (existing.fromLocalId == fromId && existing.toLocalId == toId
            && existing.fromPin == fromPin && existing.toPin == toPin) {
            return;
        }
    }

    LadderConnection connection;
    connection.fromLocalId = fromId;
    connection.toLocalId = toId;
    connection.fromPin = fromPin;
    connection.toPin = toPin;
    rung->connections.push_back(connection);
}

void LadderRung::ensureInputBranchRegions()
{
    LadderBranchLogic::migrateLegacyInputBranch(this);
    LadderBranchLogic::syncLegacyInputBranchFields(this);
}

int LadderRung::coilCount() const
{
    int count = 0;
    for (const LadderElement &element : elements) {
        if (element.kind == LadderElementKind::Coil) {
            ++count;
        }
    }
    return count;
}

bool LadderRung::hasInputBranch() const
{
    LadderRung copy = *this;
    LadderBranchLogic::migrateLegacyInputBranch(&copy);
    if (!copy.inputBranchRegions.isEmpty()) {
        return true;
    }

    int forkCol = 0;
    int joinCol = 0;
    if (!branchSpan(&forkCol, &joinCol)) {
        return false;
    }

    for (const LadderElement &element : elements) {
        if (element.row > 0 && element.kind != LadderElementKind::Coil && element.col >= forkCol &&
            element.col < joinCol) {
            return true;
        }
    }

    return false;
}

bool LadderRung::hasOutputBranch() const
{
    return outputBranchForkCol > 0 || coilCount() > 1;
}

bool LadderRung::outputBranchForkColumn(int *forkCol) const
{
    if (forkCol == nullptr) {
        return false;
    }

    if (outputBranchForkCol > 0) {
        *forkCol = outputBranchForkCol;
        return true;
    }

    if (coilCount() <= 1) {
        return false;
    }

    int minCoilCol = std::numeric_limits<int>::max();
    for (const LadderElement &element : elements) {
        if (element.kind == LadderElementKind::Coil) {
            minCoilCol = qMin(minCoilCol, element.col);
        }
    }

    *forkCol = computeOutputBranchForkCol(*this, minCoilCol);
    return true;
}

LadderElement &LadderProgram::addOutputBranchCoil(int rungIndex, LadderCoilKind kind)
{
    LadderRung *rung = rungAt(rungIndex);
    if (rung == nullptr) {
        rung = &addRung();
        rungIndex = rungs.size() - 1;
    }

    QVector<LadderElement *> existingCoils;
    for (LadderElement &element : rung->elements) {
        if (element.kind == LadderElementKind::Coil) {
            existingCoils.push_back(&element);
        }
    }

    if (existingCoils.isEmpty()) {
        return addCoil(rungIndex, 0, 1, LadderIoCatalog::outputNameForRung(rungIndex), kind);
    }

    std::sort(existingCoils.begin(), existingCoils.end(), [](const LadderElement *a, const LadderElement *b) {
        if (a->row != b->row) {
            return a->row < b->row;
        }
        return a->col < b->col;
    });

    int coilCol = existingCoils.first()->col;
    int maxCoilRow = existingCoils.first()->row;
    for (const LadderElement *coil : existingCoils) {
        coilCol = qMax(coilCol, coil->col);
        maxCoilRow = qMax(maxCoilRow, coil->row);
    }

    rung->outputBranchForkCol = computeOutputBranchForkCol(*rung, coilCol);
    rung->branchRowsBelow = qMax(rung->branchRowsBelow, maxCoilRow + 1);

    const int newRow = maxCoilRow + 1;
    return addCoil(rungIndex, newRow, coilCol,
                   LadderIoCatalog::outputNameForBranchCoil(rungIndex, existingCoils.size()), kind);
}

void LadderProgram::normalizeRungLayout(int rungIndex, int outputCoilCol)
{
    LadderRung *rung = rungAt(rungIndex);
    if (rung == nullptr) {
        return;
    }

    QMap<int, QVector<LadderElement *>> byRow;
    for (LadderElement &element : rung->elements) {
        byRow[element.row].push_back(&element);
    }

    int outputForkCol = 0;
    const bool outputBranch = rung->hasOutputBranch() && rung->outputBranchForkColumn(&outputForkCol);
    int inputForkCol = 0;
    int inputJoinCol = 0;
    const bool inputBranch = rung->hasInputBranch() && rung->branchSpan(&inputForkCol, &inputJoinCol);

    for (auto it = byRow.begin(); it != byRow.end(); ++it) {
        const int row = it.key();
        QVector<LadderElement *> &rowElements = it.value();
        QVector<LadderElement *> series;
        LadderElement *coil = nullptr;

        for (LadderElement *element : rowElements) {
            if (element->kind == LadderElementKind::Coil) {
                coil = element;
            } else {
                series.push_back(element);
            }
        }

        std::sort(series.begin(), series.end(), [](const LadderElement *a, const LadderElement *b) {
            if (a->col != b->col) {
                return a->col < b->col;
            }
            return a->localId < b->localId;
        });

        if (outputBranch && coil != nullptr && outputForkCol > 0) {
            if (row == 0) {
                QVector<LadderElement *> prefix;
                QVector<LadderElement *> outputPath;
                prefix.reserve(series.size());
                outputPath.reserve(series.size());
                for (LadderElement *element : series) {
                    if (element->col < outputForkCol) {
                        prefix.push_back(element);
                    } else {
                        outputPath.push_back(element);
                    }
                }

                int col = 1;
                for (LadderElement *element : prefix) {
                    element->col = col++;
                }
                col = outputForkCol;
                for (LadderElement *element : outputPath) {
                    element->col = col++;
                }
            } else {
                QVector<LadderElement *> inputParallel;
                QVector<LadderElement *> outputPath;
                inputParallel.reserve(series.size());
                outputPath.reserve(series.size());
                for (LadderElement *element : series) {
                    if (element->col < outputForkCol) {
                        inputParallel.push_back(element);
                    } else if (element->col < coil->col) {
                        outputPath.push_back(element);
                    }
                }

                int col = inputBranch ? inputForkCol : 1;
                for (LadderElement *element : inputParallel) {
                    element->col = col++;
                }
                col = outputForkCol;
                for (LadderElement *element : outputPath) {
                    element->col = col++;
                }
            }

            if (outputCoilCol > 0) {
                coil->col = qMax(outputCoilCol, coil->col);
            }
            continue;
        }

        int col = 1;
        for (LadderElement *element : series) {
            element->col = col++;
        }
        if (coil != nullptr) {
            const int seriesEndCol = qMax(1, col - 1);
            if (outputCoilCol > 0) {
                coil->col = qMax(seriesEndCol + 1, outputCoilCol);
            } else {
                coil->col = col;
            }
        }
    }

    LadderBranchLogic::syncInputBranchJoinCols(rung);

    QVector<LadderElement *> coils;
    for (LadderElement &element : rung->elements) {
        if (element.kind == LadderElementKind::Coil) {
            coils.push_back(&element);
        }
    }

    if (coils.size() > 1) {
        int alignedCol = outputCoilCol > 0 ? outputCoilCol : 1;
        for (const LadderElement *coil : coils) {
            alignedCol = qMax(alignedCol, coil->col);
        }
        for (LadderElement *coil : coils) {
            coil->col = alignedCol;
        }

        rung->outputBranchForkCol = computeOutputBranchForkCol(*rung, alignedCol);

        int maxCoilRow = 0;
        for (const LadderElement *coil : coils) {
            maxCoilRow = qMax(maxCoilRow, coil->row);
        }
        rung->branchRowsBelow = qMax(rung->branchRowsBelow, maxCoilRow);
    } else if (coils.size() == 1 && rung->outputBranchForkCol > 0) {
        rung->outputBranchForkCol = 0;
    }
}

void LadderProgram::normalizeAllRungs(int outputCoilCol)
{
    for (int i = 0; i < rungs.size(); ++i) {
        normalizeRungLayout(i, outputCoilCol);
    }
}

bool LadderRung::branchSpan(int *forkCol, int *joinCol) const
{
    if (forkCol == nullptr || joinCol == nullptr) {
        return false;
    }

    if (branchForkCol > 0 && branchJoinCol > branchForkCol) {
        *forkCol = branchForkCol;
        *joinCol = branchJoinCol;
        return true;
    }

    int minCol = std::numeric_limits<int>::max();
    int maxCol = 0;
    bool found = false;

    for (const LadderElement &element : elements) {
        if (element.row > 0 && element.kind != LadderElementKind::Coil) {
            found = true;
            minCol = qMin(minCol, element.col);
            maxCol = qMax(maxCol, element.col);
        }
    }

    if (!found) {
        return false;
    }

    *forkCol = minCol;
    *joinCol = maxCol + 1;
    return true;
}

bool LadderProgram::surroundOutputCoilsWithBranch(int rungIndex,
                                                  const QVector<int> &localIds,
                                                  QString *errorMessage)
{
    LadderRung *rung = rungAt(rungIndex);
    if (rung == nullptr || localIds.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Select one or more output coils to branch.");
        }
        return false;
    }

    QVector<LadderElement *> selectedCoils;
    selectedCoils.reserve(localIds.size());
    for (int localId : localIds) {
        LadderElement *element = elementById(localId);
        if (element == nullptr || element->rungIndex != rungIndex) {
            continue;
        }
        if (element->kind != LadderElementKind::Coil) {
            if (errorMessage != nullptr) {
                *errorMessage =
                    QStringLiteral("Select only output coils, or only main-line input elements — not both.");
            }
            return false;
        }
        selectedCoils.push_back(element);
    }

    if (selectedCoils.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Select one or more output coils to branch.");
        }
        return false;
    }

    std::sort(selectedCoils.begin(), selectedCoils.end(), [](const LadderElement *a, const LadderElement *b) {
        if (a->row != b->row) {
            return a->row < b->row;
        }
        return a->col < b->col;
    });

    const LadderCoilKind branchKind = selectedCoils.first()->coilKind;

    int minSelectedCol = std::numeric_limits<int>::max();
    for (const LadderElement *coil : selectedCoils) {
        minSelectedCol = qMin(minSelectedCol, coil->col);
    }

    rung->outputBranchForkCol = computeOutputBranchForkCol(*rung, minSelectedCol);

    int alignedCol = minSelectedCol;
    for (const LadderElement &element : rung->elements) {
        if (element.kind == LadderElementKind::Coil) {
            alignedCol = qMax(alignedCol, element.col);
        }
    }
    for (LadderElement *coil : selectedCoils) {
        coil->col = alignedCol;
    }

    int maxCoilRow = 0;
    for (const LadderElement &element : rung->elements) {
        if (element.kind == LadderElementKind::Coil) {
            maxCoilRow = qMax(maxCoilRow, element.row);
        }
    }
    rung->branchRowsBelow = qMax(rung->branchRowsBelow, maxCoilRow + 1);

    addOutputBranchCoil(rungIndex, branchKind);
    return true;
}

bool LadderProgram::surroundWithBranch(int rungIndex, const QVector<int> &localIds, QString *errorMessage)
{
    LadderRung *rung = rungAt(rungIndex);
    if (rung == nullptr || localIds.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Select one or more elements to surround with a branch.");
        }
        return false;
    }

    QVector<LadderElement *> selectedCoils;
    QVector<LadderElement *> selectedInputs;
    selectedCoils.reserve(localIds.size());
    selectedInputs.reserve(localIds.size());

    for (int localId : localIds) {
        LadderElement *element = elementById(localId);
        if (element == nullptr) {
            continue;
        }
        if (element->rungIndex != rungIndex) {
            continue;
        }
        if (element->kind == LadderElementKind::Coil) {
            selectedCoils.push_back(element);
        } else {
            selectedInputs.push_back(element);
        }
    }

    if (!selectedCoils.isEmpty() && !selectedInputs.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage =
                QStringLiteral("Select either output coils or main-line input elements, not both.");
        }
        return false;
    }

    if (!selectedCoils.isEmpty()) {
        return surroundOutputCoilsWithBranch(rungIndex, localIds, errorMessage);
    }

    QVector<LadderElement *> selected = selectedInputs;
    if (selected.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Select one or more elements on the same row to surround with a branch.");
        }
        return false;
    }

    const int pathRow = selected.first()->row;
    for (LadderElement *element : selected) {
        if (element->row != pathRow) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Selected elements must be on the same row.");
            }
            return false;
        }
    }

    std::sort(selected.begin(), selected.end(), [](const LadderElement *a, const LadderElement *b) {
        return a->col < b->col;
    });

    const int forkCol = selected.first()->col;
    const int joinCol = selected.last()->col + 1;

    rung->ensureInputBranchRegions();

    if (pathRow > 0) {
        const LadderInputBranchRegion *parent =
            LadderBranchLogic::inputRegionForCell(*rung, pathRow, forkCol);
        if (parent == nullptr) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Nested branches must be placed inside an existing branch region.");
            }
            return false;
        }
        LadderInputBranchRegion candidate;
        candidate.forkCol = forkCol;
        candidate.joinCol = joinCol;
        candidate.pathRow = pathRow;
        if (!LadderBranchLogic::regionNestContains(*parent, candidate)) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Nested branch must fit inside the parent branch region.");
            }
            return false;
        }
    }

    for (const LadderInputBranchRegion &existing : rung->inputBranchRegions) {
        if (existing.pathRow != pathRow) {
            continue;
        }
        if (joinCol > existing.forkCol && forkCol < existing.joinCol) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("This branch overlaps an existing branch on the same row.");
            }
            return false;
        }
    }

    LadderInputBranchRegion region;
    region.forkCol = forkCol;
    region.joinCol = joinCol;
    region.pathRow = pathRow;
    rung->inputBranchRegions.push_back(region);
    LadderBranchLogic::syncLegacyInputBranchFields(rung);

    const int regionIndex = rung->inputBranchRegions.size() - 1;
    rung->branchRowsBelow = qMax(rung->branchRowsBelow, pathRow + 1);
    addParallelBranchLeg(rungIndex, regionIndex, true, false, nullptr);
    autoWireRung(rungIndex);
    return true;
}

bool LadderProgram::removeInputBranchRegion(int rungIndex, int regionIndex, QString *errorMessage)
{
    LadderRung *rung = rungAt(rungIndex);
    if (rung == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("No rung selected.");
        }
        return false;
    }

    rung->ensureInputBranchRegions();
    if (regionIndex < 0 || regionIndex >= rung->inputBranchRegions.size()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Select a branch to remove.");
        }
        return false;
    }

    const LadderInputBranchRegion removedRegion = rung->inputBranchRegions[regionIndex];
    rung->elements.erase(std::remove_if(rung->elements.begin(),
                                        rung->elements.end(),
                                        [&](const LadderElement &element) {
                                            if (element.kind == LadderElementKind::Coil) {
                                                return false;
                                            }
                                            if (element.row <= removedRegion.pathRow) {
                                                return false;
                                            }
                                            return element.col >= removedRegion.forkCol
                                                   && element.col < removedRegion.joinCol;
                                        }),
                       rung->elements.end());

    rung->inputBranchRegions.removeAt(regionIndex);
    rung->inputBranchRegions.erase(
        std::remove_if(rung->inputBranchRegions.begin(),
                       rung->inputBranchRegions.end(),
                       [&](const LadderInputBranchRegion &region) {
                           return LadderBranchLogic::regionNestContains(removedRegion, region);
                       }),
        rung->inputBranchRegions.end());

    if (rung->inputBranchRegions.isEmpty()) {
        rung->branchForkCol = 0;
        rung->branchJoinCol = 0;
        if (!rung->hasOutputBranch()) {
            rung->branchRowsBelow = 0;
        }
    } else {
        LadderBranchLogic::syncLegacyInputBranchFields(rung);
        int maxBranchRow = 0;
        for (const LadderInputBranchRegion &region : rung->inputBranchRegions) {
            maxBranchRow = qMax(maxBranchRow, LadderBranchLogic::maxParallelRowForRegion(*rung, region));
        }
        rung->branchRowsBelow = qMax(rung->branchRowsBelow, maxBranchRow + 1);
    }

    return true;
}

bool LadderProgram::removeOutputBranch(int rungIndex, QString *errorMessage)
{
    LadderRung *rung = rungAt(rungIndex);
    if (rung == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("No rung selected.");
        }
        return false;
    }

    if (!rung->hasOutputBranch()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("This rung has no output branch.");
        }
        return false;
    }

    rung->elements.erase(std::remove_if(rung->elements.begin(),
                                          rung->elements.end(),
                                          [](const LadderElement &element) {
                                              return element.kind == LadderElementKind::Coil && element.row > 0;
                                          }),
                         rung->elements.end());
    rung->outputBranchForkCol = 0;
    if (rung->coilCount() <= 1 && !rung->hasInputBranch()) {
        rung->branchRowsBelow = 0;
    }

    return true;
}

bool LadderProgram::removeBranch(int rungIndex,
                                 int inputRegionIndex,
                                 bool removeOutput,
                                 QString *errorMessage)
{
    bool removed = false;

    if (removeOutput) {
        if (removeOutputBranch(rungIndex, errorMessage)) {
            removed = true;
        } else if (inputRegionIndex < 0) {
            return false;
        }
    }

    if (inputRegionIndex >= 0) {
        QString inputError;
        if (removeInputBranchRegion(rungIndex, inputRegionIndex, &inputError)) {
            removed = true;
        } else if (!removed) {
            if (errorMessage != nullptr) {
                *errorMessage = inputError;
            }
            return false;
        }
    }

    if (!removed) {
        LadderRung *rung = rungAt(rungIndex);
        if (rung == nullptr) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("No rung selected.");
            }
            return false;
        }

        rung->ensureInputBranchRegions();
        if (!rung->inputBranchRegions.isEmpty()) {
            const int innermostIndex = LadderBranchLogic::innermostInputRegionIndex(*rung);
            if (innermostIndex >= 0) {
                return removeInputBranchRegion(rungIndex, innermostIndex, errorMessage);
            }
        }

        int forkCol = 0;
        int joinCol = 0;
        if (rung->hasInputBranch() && rung->branchSpan(&forkCol, &joinCol)) {
            const int beforeCount = rung->elements.size();
            rung->elements.erase(std::remove_if(rung->elements.begin(),
                                                rung->elements.end(),
                                                [&](const LadderElement &element) {
                                                    return element.row > 0
                                                           && element.kind != LadderElementKind::Coil
                                                           && element.col >= forkCol && element.col < joinCol;
                                                }),
                               rung->elements.end());
            rung->branchForkCol = 0;
            rung->branchJoinCol = 0;
            if (!rung->hasOutputBranch()) {
                rung->branchRowsBelow = 0;
            }
            if (rung->elements.size() != beforeCount) {
                return true;
            }
        }

        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("This rung has no parallel branch.");
        }
        return false;
    }

    return true;
}

bool LadderProgram::addParallelBranchLeg(int rungIndex,
                                         int regionIndex,
                                         bool withContact,
                                         bool negated,
                                         QString *errorMessage)
{
    LadderRung *rung = rungAt(rungIndex);
    if (rung == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("No rung selected.");
        }
        return false;
    }

    rung->ensureInputBranchRegions();
    if (rung->inputBranchRegions.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage =
                QStringLiteral("No input branch on this rung. Select row-0 contacts and click Branch first.");
        }
        return false;
    }

    if (regionIndex < 0 || regionIndex >= rung->inputBranchRegions.size()) {
        const QVector<const LadderInputBranchRegion *> roots =
            LadderBranchLogic::rootInputRegions(*rung);
        if (roots.isEmpty()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("No input branch region found.");
            }
            return false;
        }
        regionIndex = LadderBranchLogic::inputBranchRegionIndex(*rung, *roots.first());
    }

    if (regionIndex < 0 || regionIndex >= rung->inputBranchRegions.size()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Invalid branch region.");
        }
        return false;
    }

    const LadderInputBranchRegion &region = rung->inputBranchRegions[regionIndex];
    int newRow = LadderBranchLogic::maxParallelRowForRegion(*rung, region) + 1;
    if (newRow <= region.pathRow) {
        newRow = region.pathRow + 1;
    }
    rung->branchRowsBelow = qMax(rung->branchRowsBelow, newRow);

    if (withContact) {
        LadderProgram::addContact(rungIndex, newRow, region.forkCol,
                                  LadderIoCatalog::inputNameForBranchLeg(rungIndex, newRow), negated);
    }

    autoWireRung(rungIndex);
    return true;
}

void LadderProgram::autoWireRung(int rungIndex)
{
    LadderRung *rung = rungAt(rungIndex);
    if (rung == nullptr) {
        return;
    }

    rung->connections.clear();
    rung->ensureInputBranchRegions();

    QVector<LadderElement *> coils;
    for (LadderElement &element : rung->elements) {
        if (element.kind == LadderElementKind::Coil) {
            coils.push_back(&element);
        }
    }
    std::sort(coils.begin(), coils.end(), [](const LadderElement *a, const LadderElement *b) {
        if (a->row != b->row) {
            return a->row < b->row;
        }
        return a->col < b->col;
    });

    int outputForkCol = 0;
    const bool outputBranch = rung->hasOutputBranch() && rung->outputBranchForkColumn(&outputForkCol);

    int inputForkCol = 0;
    int inputJoinCol = 0;
    const bool inputBranch = rung->hasInputBranch() && rung->branchSpan(&inputForkCol, &inputJoinCol);

    auto collectSorted = [&](int row, auto predicate) {
        QVector<LadderElement *> elements;
        for (LadderElement &element : rung->elements) {
            if (predicate(element) && element.row == row) {
                elements.push_back(&element);
            }
        }
        std::sort(elements.begin(), elements.end(), [](const LadderElement *a, const LadderElement *b) {
            return a->col < b->col;
        });
        return elements;
    };

    auto wireSeries = [&](const QVector<LadderElement *> &elements) -> LadderElement * {
        for (int i = 0; i + 1 < elements.size(); ++i) {
            connectElements(elements[i]->localId, elements[i + 1]->localId);
        }
        return elements.isEmpty() ? nullptr : elements.last();
    };

    std::function<QVector<LadderElement *>(const LadderInputBranchRegion &)> wireInputRegion;
    wireInputRegion = [&](const LadderInputBranchRegion &region) -> QVector<LadderElement *> {
        const int forkCol = region.forkCol;
        const int joinCol = region.joinCol;
        const int pathRow = region.pathRow;
        QVector<LadderElement *> orTails;

        const LadderInputBranchRegion *nested = LadderBranchLogic::nestedInputRegionOnPathRow(*rung, region);

        auto firstInSpan = [&](int row, int startCol, int endCol) -> LadderElement * {
            const QVector<LadderElement *> elements = collectSorted(row, [&](const LadderElement &element) {
                return element.kind != LadderElementKind::Coil && element.col >= startCol && element.col < endCol;
            });
            return elements.isEmpty() ? nullptr : elements.first();
        };

        if (nested != nullptr) {
            const QVector<LadderElement *> prefix = collectSorted(pathRow, [&](const LadderElement &element) {
                return element.kind != LadderElementKind::Coil && element.col >= forkCol &&
                       element.col < nested->forkCol;
            });
            LadderElement *prefixTail = wireSeries(prefix);
            QVector<LadderElement *> innerTails = wireInputRegion(*nested);
            const QVector<LadderElement *> suffix = collectSorted(pathRow, [&](const LadderElement &element) {
                return element.kind != LadderElementKind::Coil && element.col >= nested->joinCol &&
                       element.col < joinCol;
            });
            LadderElement *suffixHead = suffix.isEmpty() ? nullptr : suffix.first();
            LadderElement *suffixTail = wireSeries(suffix);

            if (prefixTail != nullptr) {
                LadderElement *innerEntry = firstInSpan(nested->pathRow, nested->forkCol, nested->joinCol);
                if (innerEntry != nullptr) {
                    connectElements(prefixTail->localId, innerEntry->localId);
                }
            }

            if (innerTails.isEmpty() && suffixTail != nullptr) {
                if (prefixTail != nullptr && suffixHead != nullptr) {
                    connectElements(prefixTail->localId, suffixHead->localId);
                }
                orTails.push_back(suffixTail);
            } else {
                for (LadderElement *tail : innerTails) {
                    if (suffixHead != nullptr) {
                        connectElements(tail->localId, suffixHead->localId);
                        orTails.push_back(suffixTail != nullptr ? suffixTail : tail);
                    } else {
                        orTails.push_back(tail);
                    }
                }
            }
            if (innerTails.isEmpty() && suffix.isEmpty() && prefixTail != nullptr) {
                orTails.push_back(prefixTail);
            }
        } else {
            const QVector<LadderElement *> spanMain = collectSorted(pathRow, [&](const LadderElement &element) {
                return element.kind != LadderElementKind::Coil && element.col >= forkCol &&
                       element.col < joinCol;
            });
            LadderElement *spanMainTail = wireSeries(spanMain);
            if (spanMainTail != nullptr) {
                orTails.push_back(spanMainTail);
            }
        }

        const int maxRow = LadderBranchLogic::maxParallelRowForRegion(*rung, region);
        for (int row = pathRow + 1; row <= maxRow; ++row) {
            const LadderInputBranchRegion *child =
                LadderBranchLogic::childInputRegionOnRow(*rung, region, row);
            if (child != nullptr) {
                orTails.append(wireInputRegion(*child));
            } else {
                const QVector<LadderElement *> branchSpan = collectSorted(row, [&](const LadderElement &element) {
                    return element.kind != LadderElementKind::Coil && element.col >= forkCol &&
                           element.col < joinCol;
                });
                LadderElement *branchTail = wireSeries(branchSpan);
                if (branchTail != nullptr) {
                    orTails.push_back(branchTail);
                }
            }
        }

        return orTails;
    };

    auto wireInputOrToTarget = [&](const QVector<LadderElement *> &orTails, LadderElement *targetFirst) {
        if (targetFirst == nullptr) {
            return;
        }
        for (LadderElement *tail : orTails) {
            connectElements(tail->localId, targetFirst->localId);
        }
    };

    const bool nestedInputRegions = !rung->inputBranchRegions.isEmpty();
    if (nestedInputRegions && inputBranch) {
        const QVector<const LadderInputBranchRegion *> roots = LadderBranchLogic::rootInputRegions(*rung);
        if (!roots.isEmpty()) {
            const LadderInputBranchRegion &root = *roots.first();
            inputForkCol = root.forkCol;
            inputJoinCol = root.joinCol;

            const QVector<LadderElement *> prefix = collectSorted(0, [&](const LadderElement &element) {
                return element.kind != LadderElementKind::Coil && element.col < inputForkCol;
            });
            const int suffixEndCol = outputBranch ? outputForkCol : (coils.isEmpty() ? inputJoinCol : coils.first()->col);
            const QVector<LadderElement *> suffix = collectSorted(0, [&](const LadderElement &element) {
                return element.kind != LadderElementKind::Coil && element.col >= inputJoinCol &&
                       element.col < suffixEndCol;
            });

            LadderElement *prefixTail = wireSeries(prefix);
            QVector<LadderElement *> orTails = wireInputRegion(root);
            if (prefixTail != nullptr) {
                LadderElement *rootEntry = collectSorted(root.pathRow, [&](const LadderElement &element) {
                    return element.kind != LadderElementKind::Coil && element.col >= root.forkCol &&
                           element.col < root.joinCol;
                }).value(0, nullptr);
                if (rootEntry != nullptr) {
                    connectElements(prefixTail->localId, rootEntry->localId);
                }
            }

            LadderElement *suffixTail = wireSeries(suffix);
            if (outputBranch && !coils.isEmpty()) {
                if (!suffix.isEmpty()) {
                    wireInputOrToTarget(orTails, suffix.first());
                }
                LadderElement *feedTail = suffixTail;
                if (feedTail == nullptr && !orTails.isEmpty()) {
                    feedTail = orTails.first();
                }
                for (LadderElement *coil : coils) {
                    const QVector<LadderElement *> rowPath =
                        collectSorted(coil->row, [&](const LadderElement &element) {
                            return element.kind != LadderElementKind::Coil && element.col >= outputForkCol &&
                                   element.col < coil->col;
                        });
                    LadderElement *rowPathTail = wireSeries(rowPath);
                    if (rowPathTail != nullptr) {
                        if (feedTail != nullptr) {
                            connectElements(feedTail->localId, rowPath.first()->localId);
                        } else {
                            wireInputOrToTarget(orTails, rowPath.first());
                        }
                        connectElements(rowPathTail->localId, coil->localId);
                    } else if (feedTail != nullptr) {
                        connectElements(feedTail->localId, coil->localId);
                    } else {
                        wireInputOrToTarget(orTails, coil);
                    }
                }
            } else if (!suffix.isEmpty()) {
                wireInputOrToTarget(orTails, suffix.first());
                for (LadderElement *coilTarget : coils) {
                    if (suffixTail != nullptr) {
                        connectElements(suffixTail->localId, coilTarget->localId);
                    }
                }
            } else if (!coils.isEmpty()) {
                for (LadderElement *coilTarget : coils) {
                    wireInputOrToTarget(orTails, coilTarget);
                }
            }
            return;
        }
    }

    if (inputBranch && outputBranch && !coils.isEmpty()) {
        const QVector<LadderElement *> prefix = collectSorted(0, [&](const LadderElement &element) {
            return element.kind != LadderElementKind::Coil && element.col < inputForkCol;
        });
        const QVector<LadderElement *> spanMain = collectSorted(0, [&](const LadderElement &element) {
            return element.kind != LadderElementKind::Coil && element.col >= inputForkCol &&
                   element.col < inputJoinCol;
        });
        const QVector<LadderElement *> suffix = collectSorted(0, [&](const LadderElement &element) {
            return element.kind != LadderElementKind::Coil && element.col >= inputJoinCol &&
                   element.col < outputForkCol;
        });

        LadderElement *prefixTail = wireSeries(prefix);
        LadderElement *spanMainTail = wireSeries(spanMain);
        if (prefixTail != nullptr && !spanMain.isEmpty()) {
            connectElements(prefixTail->localId, spanMain.first()->localId);
        }

        QVector<LadderElement *> orTails;
        if (spanMainTail != nullptr) {
            orTails.push_back(spanMainTail);
        }

        QSet<int> branchRows;
        for (const LadderElement &element : rung->elements) {
            if (element.row > 0 && element.kind != LadderElementKind::Coil && element.col >= inputForkCol &&
                element.col < inputJoinCol) {
                branchRows.insert(element.row);
            }
        }
        for (int row : branchRows) {
            const QVector<LadderElement *> branchSpan = collectSorted(row, [&](const LadderElement &element) {
                return element.kind != LadderElementKind::Coil && element.col >= inputForkCol &&
                       element.col < inputJoinCol;
            });
            LadderElement *branchTail = wireSeries(branchSpan);
            if (branchTail != nullptr) {
                orTails.push_back(branchTail);
            }
        }

        LadderElement *suffixTail = wireSeries(suffix);
        LadderElement *feedTail = suffixTail;
        if (feedTail == nullptr && !orTails.isEmpty()) {
            feedTail = orTails.first();
        }

        for (LadderElement *coil : coils) {
            const QVector<LadderElement *> rowPath = collectSorted(coil->row, [&](const LadderElement &element) {
                return element.kind != LadderElementKind::Coil && element.col >= outputForkCol &&
                       element.col < coil->col;
            });
            LadderElement *rowPathTail = wireSeries(rowPath);
            if (rowPathTail != nullptr) {
                if (feedTail != nullptr) {
                    connectElements(feedTail->localId, rowPath.first()->localId);
                } else {
                    for (LadderElement *tail : orTails) {
                        connectElements(tail->localId, rowPath.first()->localId);
                    }
                }
                connectElements(rowPathTail->localId, coil->localId);
            } else if (feedTail != nullptr) {
                connectElements(feedTail->localId, coil->localId);
            } else {
                for (LadderElement *tail : orTails) {
                    connectElements(tail->localId, coil->localId);
                }
            }
        }
    } else if (outputBranch && !coils.isEmpty()) {
        QVector<LadderElement *> prefix;
        for (LadderElement &element : rung->elements) {
            if (element.row == 0 && element.kind != LadderElementKind::Coil && element.col < outputForkCol) {
                prefix.push_back(&element);
            }
        }
        std::sort(prefix.begin(), prefix.end(), [](const LadderElement *a, const LadderElement *b) {
            return a->col < b->col;
        });
        for (int i = 0; i + 1 < prefix.size(); ++i) {
            connectElements(prefix[i]->localId, prefix[i + 1]->localId);
        }

        LadderElement *prefixTail = prefix.isEmpty() ? nullptr : prefix.last();
        for (LadderElement *coil : coils) {
            QVector<LadderElement *> rowPath;
            for (LadderElement &element : rung->elements) {
                if (element.row == coil->row && element.kind != LadderElementKind::Coil &&
                    element.col >= outputForkCol && element.col < coil->col) {
                    rowPath.push_back(&element);
                }
            }
            std::sort(rowPath.begin(), rowPath.end(), [](const LadderElement *a, const LadderElement *b) {
                return a->col < b->col;
            });
            for (int i = 0; i + 1 < rowPath.size(); ++i) {
                connectElements(rowPath[i]->localId, rowPath[i + 1]->localId);
            }

            if (rowPath.isEmpty()) {
                if (prefixTail != nullptr) {
                    connectElements(prefixTail->localId, coil->localId);
                }
            } else {
                if (prefixTail != nullptr) {
                    connectElements(prefixTail->localId, rowPath.first()->localId);
                }
                connectElements(rowPath.last()->localId, coil->localId);
            }
        }
    } else if (inputBranch && !coils.isEmpty()) {
        LadderElement *coil = coils.first();

        const QVector<LadderElement *> prefix = collectSorted(0, [&](const LadderElement &element) {
            return element.kind != LadderElementKind::Coil && element.col < inputForkCol;
        });
        const QVector<LadderElement *> spanMain = collectSorted(0, [&](const LadderElement &element) {
            return element.kind != LadderElementKind::Coil && element.col >= inputForkCol &&
                   element.col < inputJoinCol;
        });
        const int suffixEndCol = outputBranch ? outputForkCol : coil->col;
        const QVector<LadderElement *> suffix = collectSorted(0, [&](const LadderElement &element) {
            return element.kind != LadderElementKind::Coil && element.col >= inputJoinCol &&
                   element.col < suffixEndCol;
        });

        LadderElement *prefixTail = wireSeries(prefix);
        LadderElement *spanMainTail = wireSeries(spanMain);
        if (prefixTail != nullptr && !spanMain.isEmpty()) {
            connectElements(prefixTail->localId, spanMain.first()->localId);
        }

        QVector<LadderElement *> orTails;
        if (spanMainTail != nullptr) {
            orTails.push_back(spanMainTail);
        }

        QSet<int> branchRows;
        for (const LadderElement &element : rung->elements) {
            if (element.row > 0 && element.kind != LadderElementKind::Coil) {
                branchRows.insert(element.row);
            }
        }
        for (int row : branchRows) {
            const QVector<LadderElement *> branchSpan = collectSorted(row, [&](const LadderElement &element) {
                return element.kind != LadderElementKind::Coil && element.col >= inputForkCol &&
                       element.col < inputJoinCol;
            });
            LadderElement *branchTail = wireSeries(branchSpan);
            if (branchTail != nullptr) {
                orTails.push_back(branchTail);
            }
        }

        LadderElement *suffixTail = wireSeries(suffix);
        if (!suffix.isEmpty()) {
            for (LadderElement *tail : orTails) {
                connectElements(tail->localId, suffix.first()->localId);
            }
            for (LadderElement *coilTarget : coils) {
                connectElements(suffixTail->localId, coilTarget->localId);
            }
        } else {
            for (LadderElement *coilTarget : coils) {
                for (LadderElement *tail : orTails) {
                    connectElements(tail->localId, coilTarget->localId);
                }
            }
        }
    } else if (inputBranch && coils.isEmpty()) {
        const QVector<LadderElement *> prefix = collectSorted(0, [&](const LadderElement &element) {
            return element.kind != LadderElementKind::Coil && element.col < inputForkCol;
        });
        const QVector<LadderElement *> spanMain = collectSorted(0, [&](const LadderElement &element) {
            return element.kind != LadderElementKind::Coil && element.col >= inputForkCol &&
                   element.col < inputJoinCol;
        });
        LadderElement *prefixTail = wireSeries(prefix);
        LadderElement *spanMainTail = wireSeries(spanMain);
        if (prefixTail != nullptr && !spanMain.isEmpty()) {
            connectElements(prefixTail->localId, spanMain.first()->localId);
        }

        QSet<int> branchRows;
        for (const LadderElement &element : rung->elements) {
            if (element.row > 0 && element.kind != LadderElementKind::Coil) {
                branchRows.insert(element.row);
            }
        }
        for (int row : branchRows) {
            const QVector<LadderElement *> branchSpan = collectSorted(row, [&](const LadderElement &element) {
                return element.kind != LadderElementKind::Coil && element.col >= inputForkCol &&
                       element.col < inputJoinCol;
            });
            wireSeries(branchSpan);
        }
        Q_UNUSED(spanMainTail);
    } else {
        LadderElement *coil = coils.isEmpty() ? nullptr : coils.first();
        QVector<LadderElement *> mainSeries;
        for (LadderElement &element : rung->elements) {
            if (element.row != 0) {
                continue;
            }
            if (element.kind == LadderElementKind::Coil) {
                coil = &element;
            } else {
                mainSeries.push_back(&element);
            }
        }

        std::sort(mainSeries.begin(), mainSeries.end(), [](const LadderElement *a, const LadderElement *b) {
            return a->col < b->col;
        });

        for (int i = 0; i + 1 < mainSeries.size(); ++i) {
            connectElements(mainSeries[i]->localId, mainSeries[i + 1]->localId);
        }
        if (!mainSeries.isEmpty() && coil != nullptr) {
            connectElements(mainSeries.last()->localId, coil->localId);
        }
    }
}

void LadderProgram::clear()
{
    name = QStringLiteral("LDProgram");
    variables.clear();
    rungs.clear();
    nextLocalId = 1;
}

void LadderProgram::newDefaultProgram()
{
    clear();
    ensureBoardIoVariables();
}

bool LadderModel::isTimerFunctionBlock(const QString &typeName)
{
    return typeName.compare(QStringLiteral("TON"), Qt::CaseInsensitive) == 0
           || typeName.compare(QStringLiteral("TOF"), Qt::CaseInsensitive) == 0
           || typeName.compare(QStringLiteral("TP"), Qt::CaseInsensitive) == 0;
}

bool LadderModel::isRungWiredInputPin(const QString &pinName)
{
    return pinName.compare(QStringLiteral("IN"), Qt::CaseInsensitive) == 0
           || pinName.compare(QStringLiteral("CU"), Qt::CaseInsensitive) == 0
           || pinName.compare(QStringLiteral("CD"), Qt::CaseInsensitive) == 0
           || pinName.compare(QStringLiteral("CLK"), Qt::CaseInsensitive) == 0
           || pinName.compare(QStringLiteral("S1"), Qt::CaseInsensitive) == 0
           || pinName.compare(QStringLiteral("S"), Qt::CaseInsensitive) == 0
           || pinName.compare(QStringLiteral("R"), Qt::CaseInsensitive) == 0
           || pinName.compare(QStringLiteral("R1"), Qt::CaseInsensitive) == 0
           || pinName.compare(QStringLiteral("LD"), Qt::CaseInsensitive) == 0;
}

QString LadderModel::functionBlockSubLabel(const LadderElement &element)
{
    if (element.kind != LadderElementKind::FunctionBlock) {
        return QString();
    }

    QString pinName;
    if (isTimerFunctionBlock(element.fbTypeName)) {
        pinName = QStringLiteral("PT");
    } else if (element.fbTypeName.compare(QStringLiteral("CTU"), Qt::CaseInsensitive) == 0
               || element.fbTypeName.compare(QStringLiteral("CTD"), Qt::CaseInsensitive) == 0
               || element.fbTypeName.compare(QStringLiteral("CTUD"), Qt::CaseInsensitive) == 0) {
        pinName = QStringLiteral("PV");
    } else {
        return QString();
    }

    for (const LadderPinBinding &binding : element.pinBindings) {
        if (binding.formalName.compare(pinName, Qt::CaseInsensitive) != 0) {
            continue;
        }
        if (!binding.literal.isEmpty()) {
            return binding.literal;
        }
        if (!binding.variable.isEmpty()) {
            return binding.variable;
        }
    }

    for (const LadderFbDescriptor &desc : functionBlockCatalog()) {
        if (desc.typeName.compare(element.fbTypeName, Qt::CaseInsensitive) == 0) {
            return desc.defaultLiterals.value(pinName);
        }
    }

    return QString();
}

bool LadderModel::isConfigurableInputPin(const QString &typeName, const QString &pinName)
{
    if (isRungWiredInputPin(pinName)) {
        return false;
    }
    if (isTimerFunctionBlock(typeName)) {
        return pinName.compare(QStringLiteral("PT"), Qt::CaseInsensitive) == 0;
    }
    for (const LadderFbDescriptor &desc : functionBlockCatalog()) {
        if (desc.typeName.compare(typeName, Qt::CaseInsensitive) != 0) {
            continue;
        }
        return desc.inputPins.contains(pinName, Qt::CaseInsensitive);
    }
    return false;
}

const QVector<LadderFbDescriptor> &LadderModel::functionBlockCatalog()
{
    static const QVector<LadderFbDescriptor> catalog = {
        {QStringLiteral("TON"), {QStringLiteral("IN"), QStringLiteral("PT")}, {QStringLiteral("Q")}, {{QStringLiteral("PT"), QStringLiteral("T#1s")}}},
        {QStringLiteral("TOF"), {QStringLiteral("IN"), QStringLiteral("PT")}, {QStringLiteral("Q")}, {{QStringLiteral("PT"), QStringLiteral("T#1s")}}},
        {QStringLiteral("TP"), {QStringLiteral("IN"), QStringLiteral("PT")}, {QStringLiteral("Q")}, {{QStringLiteral("PT"), QStringLiteral("T#500ms")}}},
        {QStringLiteral("CTU"), {QStringLiteral("CU"), QStringLiteral("R"), QStringLiteral("PV")}, {QStringLiteral("Q"), QStringLiteral("CV")}, {{QStringLiteral("PV"), QStringLiteral("10")}}},
        {QStringLiteral("CTD"), {QStringLiteral("CD"), QStringLiteral("LD"), QStringLiteral("PV")}, {QStringLiteral("Q"), QStringLiteral("CV")}, {{QStringLiteral("PV"), QStringLiteral("10")}}},
        {QStringLiteral("CTUD"), {QStringLiteral("CU"), QStringLiteral("CD"), QStringLiteral("R"), QStringLiteral("LD"), QStringLiteral("PV")}, {QStringLiteral("QU"), QStringLiteral("QD"), QStringLiteral("CV")}, {{QStringLiteral("PV"), QStringLiteral("10")}}},
        {QStringLiteral("R_TRIG"), {QStringLiteral("CLK")}, {QStringLiteral("Q")}, {}},
        {QStringLiteral("F_TRIG"), {QStringLiteral("CLK")}, {QStringLiteral("Q")}, {}},
        {QStringLiteral("SR"), {QStringLiteral("S1"), QStringLiteral("R")}, {QStringLiteral("Q1")}, {}},
        {QStringLiteral("RS"), {QStringLiteral("S"), QStringLiteral("R1")}, {QStringLiteral("Q1")}, {}},
    };
    return catalog;
}

bool LadderModel::loadFromPath(const QString &path, QString *errorMessage)
{
    return LadderPlcopenSerializer::load(path, program, errorMessage);
}

bool LadderModel::saveToPath(const QString &path, QString *errorMessage) const
{
    return LadderPlcopenSerializer::save(path, program, errorMessage);
}
