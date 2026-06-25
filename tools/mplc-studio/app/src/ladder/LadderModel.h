/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderBranchLogic.h"

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>

enum class LadderVarType {
    Bool,
    Int,
    Time
};

enum class LadderCoilKind {
    Normal,
    Set,
    Reset
};

enum class LadderElementKind {
    Contact,
    Coil,
    FunctionBlock
};

struct LadderVariable {
    QString name;
    LadderVarType type = LadderVarType::Bool;
    QString ioAddress;
};

struct LadderPinBinding {
    QString formalName;
    QString variable;
    QString literal;
};

struct LadderElement {
    int localId = 0;
    LadderElementKind kind = LadderElementKind::Contact;
    int rungIndex = 0;
    int row = 0;
    int col = 0;

    QString variable;
    bool negated = false;
    LadderCoilKind coilKind = LadderCoilKind::Normal;

    QString fbTypeName;
    QVector<LadderPinBinding> pinBindings;
};

struct LadderConnection {
    int fromLocalId = 0;
    int toLocalId = 0;
    QString fromPin;
    QString toPin;
};

struct LadderRung {
    int localId = 0;
    QVector<LadderElement> elements;
    QVector<LadderConnection> connections;
    int branchForkCol = 0;
    int branchJoinCol = 0;
    int branchRowsBelow = 0;
    int outputBranchForkCol = 0;
    QVector<LadderInputBranchRegion> inputBranchRegions;

    int coilCount() const;
    bool hasInputBranch() const;
    bool hasOutputBranch() const;
    bool outputBranchForkColumn(int *forkCol) const;
    bool branchSpan(int *forkCol, int *joinCol) const;
    void ensureInputBranchRegions();
};

struct LadderProgram {
    QString name = QStringLiteral("LDProgram");
    QVector<LadderVariable> variables;
    QVector<LadderRung> rungs;
    int nextLocalId = 1;

    int allocateLocalId();
    LadderRung *rungAt(int index);
    const LadderRung *rungAt(int index) const;
    LadderElement *elementById(int localId);
    const LadderElement *elementById(int localId) const;
    void ensureBoardIoVariables();
    void addVariableIfMissing(const QString &name, LadderVarType type = LadderVarType::Bool);
    LadderRung &addRung();
    void removeRung(int index);
    LadderElement &addContact(int rungIndex, int row, int col, const QString &variable, bool negated = false);
    LadderElement &addCoil(int rungIndex, int row, int col, const QString &variable,
                           LadderCoilKind kind = LadderCoilKind::Normal);
    LadderElement &addOutputBranchCoil(int rungIndex, LadderCoilKind kind = LadderCoilKind::Normal);
    LadderElement &addFunctionBlock(int rungIndex, int row, int col, const QString &typeName);
    void connectElements(int fromId, int toId, const QString &fromPin = QString(),
                         const QString &toPin = QString());
    void normalizeRungLayout(int rungIndex, int outputCoilCol = 0);
    void normalizeAllRungs(int outputCoilCol = 0);
    void autoWireRung(int rungIndex);
    bool surroundWithBranch(int rungIndex, const QVector<int> &localIds, QString *errorMessage = nullptr);
    bool surroundOutputCoilsWithBranch(int rungIndex, const QVector<int> &localIds, QString *errorMessage = nullptr);
    bool removeInputBranchRegion(int rungIndex, int regionIndex, QString *errorMessage = nullptr);
    bool removeOutputBranch(int rungIndex, QString *errorMessage = nullptr);
    bool removeBranch(int rungIndex, int inputRegionIndex, bool removeOutput, QString *errorMessage = nullptr);
    bool addParallelBranchLeg(int rungIndex, int regionIndex, bool withContact, bool negated,
                              QString *errorMessage = nullptr);
    void clear();
    void newDefaultProgram();
};

struct LadderFbDescriptor {
    QString typeName;
    QStringList inputPins;
    QStringList outputPins;
    QMap<QString, QString> defaultLiterals;
};

class LadderModel
{
public:
    static const QVector<LadderFbDescriptor> &functionBlockCatalog();
    static bool isTimerFunctionBlock(const QString &typeName);
    static bool isRungWiredInputPin(const QString &pinName);
    static bool isConfigurableInputPin(const QString &typeName, const QString &pinName);
    static QString functionBlockSubLabel(const LadderElement &element);

    LadderProgram program;

    bool loadFromPath(const QString &path, QString *errorMessage);
    bool saveToPath(const QString &path, QString *errorMessage) const;
};
