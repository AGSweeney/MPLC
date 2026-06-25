/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "StLadderImporter.h"
#include "LadderValidator.h"

#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QVector>

namespace {

struct StExpr {
    enum Type { Empty, Ident, Not, And, Or, TrueLit, FalseLit };
    Type type = Empty;
    QString ident;
    QVector<StExpr> args;
};

struct FbInstance {
    QString instanceName;
    QString typeName;
};

struct FbCall {
    QString instanceName;
    QMap<QString, QString> assignments;
};

struct CoilStatement {
    QString coilVariable;
    LadderCoilKind coilKind = LadderCoilKind::Normal;
    StExpr condition;
    QVector<FbCall> fbCalls;
};

class StExprParser
{
public:
    explicit StExprParser(const QString &input)
        : m_input(input)
    {
    }

    StExpr parse()
    {
        StExpr result = parseOr();
        skipSpace();
        return result;
    }

private:
    QString m_input;
    int m_pos = 0;

    void skipSpace()
    {
        while (m_pos < m_input.size() && m_input[m_pos].isSpace()) {
            ++m_pos;
        }
    }

    bool matchKeyword(const QString &word)
    {
        skipSpace();
        if (m_input.mid(m_pos, word.size()).compare(word, Qt::CaseInsensitive) != 0) {
            return false;
        }
        const int end = m_pos + word.size();
        if (end < m_input.size()) {
            const QChar next = m_input[end];
            if (next.isLetterOrNumber() || next == QLatin1Char('_')) {
                return false;
            }
        }
        if (m_pos > 0) {
            const QChar prev = m_input[m_pos - 1];
            if (prev.isLetterOrNumber() || prev == QLatin1Char('_')) {
                return false;
            }
        }
        m_pos = end;
        return true;
    }

    QString parseIdent()
    {
        skipSpace();
        int start = m_pos;
        while (m_pos < m_input.size()) {
            const QChar ch = m_input[m_pos];
            if (ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char('.')) {
                ++m_pos;
            } else {
                break;
            }
        }
        return m_input.mid(start, m_pos - start);
    }

    StExpr parsePrimary()
    {
        skipSpace();
        if (matchKeyword(QStringLiteral("NOT"))) {
            StExpr node;
            node.type = StExpr::Not;
            node.args.push_back(parsePrimary());
            return node;
        }
        if (m_pos < m_input.size() && m_input[m_pos] == QLatin1Char('(')) {
            ++m_pos;
            StExpr inner = parseOr();
            skipSpace();
            if (m_pos < m_input.size() && m_input[m_pos] == QLatin1Char(')')) {
                ++m_pos;
            }
            return inner;
        }
        if (matchKeyword(QStringLiteral("TRUE"))) {
            StExpr node;
            node.type = StExpr::TrueLit;
            return node;
        }
        if (matchKeyword(QStringLiteral("FALSE"))) {
            StExpr node;
            node.type = StExpr::FalseLit;
            return node;
        }

        const QString ident = parseIdent();
        StExpr node;
        node.type = StExpr::Ident;
        node.ident = ident;
        return node;
    }

    StExpr parseAnd()
    {
        StExpr left = parsePrimary();
        skipSpace();
        while (matchKeyword(QStringLiteral("AND"))) {
            StExpr right = parsePrimary();
            if (left.type == StExpr::And) {
                left.args.push_back(right);
            } else {
                StExpr combined;
                combined.type = StExpr::And;
                combined.args = {left, right};
                left = combined;
            }
            skipSpace();
        }
        return left;
    }

    StExpr parseOr()
    {
        StExpr left = parseAnd();
        skipSpace();
        while (matchKeyword(QStringLiteral("OR"))) {
            StExpr right = parseAnd();
            if (left.type == StExpr::Or) {
                left.args.push_back(right);
            } else {
                StExpr combined;
                combined.type = StExpr::Or;
                combined.args = {left, right};
                left = combined;
            }
            skipSpace();
        }
        return left;
    }
};

QString stripComments(const QString &source)
{
    QString result;
    result.reserve(source.size());

    int i = 0;
    while (i < source.size()) {
        if (source[i] == QLatin1Char('(') && i + 1 < source.size() && source[i + 1] == QLatin1Char('*')) {
            i += 2;
            while (i + 1 < source.size() && !(source[i] == QLatin1Char('*') && source[i + 1] == QLatin1Char(')'))) {
                ++i;
            }
            i += 2;
            continue;
        }
        if (source[i] == QLatin1Char('#')) {
            while (i < source.size() && source[i] != QLatin1Char('\n')) {
                ++i;
            }
            continue;
        }
        result.append(source[i]);
        ++i;
    }
    return result;
}

QString normalizeSpaces(QString text)
{
    text.replace(QRegularExpression(QStringLiteral("[\\r\\n]+")), QStringLiteral("\n"));
    return text.trimmed();
}

bool isFbTypeName(const QString &typeName)
{
    for (const LadderFbDescriptor &desc : LadderModel::functionBlockCatalog()) {
        if (desc.typeName.compare(typeName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

LadderVarType parseVarType(const QString &typeName)
{
    if (typeName.compare(QStringLiteral("INT"), Qt::CaseInsensitive) == 0
        || typeName.compare(QStringLiteral("DINT"), Qt::CaseInsensitive) == 0) {
        return LadderVarType::Int;
    }
    if (typeName.compare(QStringLiteral("TIME"), Qt::CaseInsensitive) == 0) {
        return LadderVarType::Time;
    }
    return LadderVarType::Bool;
}

QString extractBlock(const QString &source, const QString &startToken, const QString &endToken)
{
    const QRegularExpression re(QStringLiteral("(?i)\\b%1\\b").arg(startToken));
    const QRegularExpression endRe(QStringLiteral("(?i)\\b%1\\b").arg(endToken));
    const QRegularExpressionMatch startMatch = re.match(source);
    if (!startMatch.hasMatch()) {
        return QString();
    }

    int depth = 1;
    int pos = startMatch.capturedEnd();
    int bodyStart = pos;
    while (pos < source.size()) {
        const QRegularExpressionMatch endMatch = endRe.match(source, pos);
        const QRegularExpressionMatch openMatch = re.match(source, pos);
        if (openMatch.hasMatch() && (!endMatch.hasMatch() || openMatch.capturedStart() < endMatch.capturedStart())) {
            ++depth;
            pos = openMatch.capturedEnd();
            continue;
        }
        if (endMatch.hasMatch()) {
            --depth;
            if (depth == 0) {
                return source.mid(bodyStart, endMatch.capturedStart() - bodyStart).trimmed();
            }
            pos = endMatch.capturedEnd();
            continue;
        }
        break;
    }
    return source.mid(bodyStart).trimmed();
}

QString extractProgramName(const QString &source)
{
    static const QRegularExpression re(QStringLiteral("(?i)\\bPROGRAM\\s+(\\w+)"));
    const QRegularExpressionMatch match = re.match(source);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return QStringLiteral("LDProgram");
}

QString mergeVarBlocks(const QString &source)
{
    QString merged;
    int searchFrom = 0;
    static const QRegularExpression varRe(QStringLiteral("(?i)\\bVAR\\b"));
    while (true) {
        const QRegularExpressionMatch match = varRe.match(source, searchFrom);
        if (!match.hasMatch()) {
            break;
        }
        const QString block = extractBlock(source.mid(match.capturedStart()), QStringLiteral("VAR"), QStringLiteral("END_VAR"));
        if (!block.isEmpty()) {
            if (!merged.isEmpty()) {
                merged.append(QLatin1Char('\n'));
            }
            merged.append(block);
        }
        searchFrom = match.capturedEnd();
    }
    return merged;
}

QString extractProgramBody(const QString &source)
{
    static const QRegularExpression re(QStringLiteral("(?i)\\bPROGRAM\\s+\\w+\\b"));
    const QRegularExpressionMatch match = re.match(source);
    if (!match.hasMatch()) {
        return source;
    }

    const QString afterProgram = source.mid(match.capturedEnd()).trimmed();
    if (afterProgram.startsWith(QStringLiteral("VAR"), Qt::CaseInsensitive)) {
        static const QRegularExpression endVar(QStringLiteral("(?i)\\bEND_VAR\\b"));
        const QRegularExpressionMatch endMatch = endVar.match(afterProgram);
        if (endMatch.hasMatch()) {
            return afterProgram.mid(endMatch.capturedEnd()).trimmed();
        }
    }

    const QString untilEnd = extractBlock(source.mid(match.capturedStart()), QStringLiteral("PROGRAM"), QStringLiteral("END_PROGRAM"));
    if (!untilEnd.isEmpty()) {
        static const QRegularExpression endVar(QStringLiteral("(?i)\\bEND_VAR\\b"));
        const QRegularExpressionMatch endMatch = endVar.match(untilEnd);
        if (endMatch.hasMatch()) {
            return untilEnd.mid(endMatch.capturedEnd()).trimmed();
        }
        return untilEnd;
    }
    return afterProgram;
}

void parseVariables(const QString &varBlock, LadderProgram *program, QMap<QString, FbInstance> *fbInstances,
                    QStringList *diagnostics)
{
    const QStringList lines = varBlock.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    static const QRegularExpression declRe(
        QStringLiteral("^\\s*([A-Za-z_]\\w*)\\s*(?:AT\\s+(\\S+))?\\s*:\\s*([A-Za-z_]\\w*)\\s*;\\s*$"),
        QRegularExpression::CaseInsensitiveOption);

    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const QRegularExpressionMatch match = declRe.match(line);
        if (!match.hasMatch()) {
            diagnostics->push_back(QStringLiteral("Skipped variable declaration: %1").arg(line));
            continue;
        }

        const QString name = match.captured(1);
        const QString address = match.captured(2);
        const QString typeName = match.captured(3);

        if (isFbTypeName(typeName)) {
            FbInstance instance;
            instance.instanceName = name;
            instance.typeName = typeName.toUpper();
            fbInstances->insert(name, instance);
            continue;
        }

        LadderVariable var;
        var.name = name;
        var.type = parseVarType(typeName);
        var.ioAddress = address;
        program->variables.push_back(var);
    }
}

QStringList splitTopLevelStatements(const QString &body)
{
    QStringList statements;
    QString current;
    int ifDepth = 0;

    auto flush = [&]() {
        const QString trimmed = current.trimmed();
        if (!trimmed.isEmpty()) {
            statements.push_back(trimmed);
        }
        current.clear();
    };

    for (int i = 0; i < body.size(); ++i) {
        const QChar ch = body[i];
        if (ch == QLatin1Char(';') && ifDepth == 0) {
            current.append(ch);
            flush();
            continue;
        }
        current.append(ch);

        if (body.mid(i, 2).compare(QStringLiteral("IF"), Qt::CaseInsensitive) == 0
            && (i == 0 || !body[i - 1].isLetterOrNumber())) {
            ifDepth++;
            ++i;
            current.append(body[i]);
            continue;
        }
        if (body.mid(i, 5).compare(QStringLiteral("END_IF"), Qt::CaseInsensitive) == 0
            && (i == 0 || !body[i - 1].isLetterOrNumber())) {
            ifDepth = qMax(0, ifDepth - 1);
            i += 4;
            for (int j = 0; j < 5; ++j) {
                if (i - 4 + j < body.size()) {
                    current.append(body[i - 4 + j]);
                }
            }
        }
    }

    flush();
    return statements;
}

bool parseFbCall(const QString &statement, FbCall *call)
{
    static const QRegularExpression re(
        QStringLiteral("^\\s*([A-Za-z_]\\w*)\\s*\\((.*)\\)\\s*;\\s*$"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = re.match(statement.trimmed());
    if (!match.hasMatch()) {
        return false;
    }

    call->instanceName = match.captured(1);
    const QString argsText = match.captured(2).trimmed();
    if (argsText.isEmpty()) {
        return true;
    }

    const QStringList parts = argsText.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const int assignPos = part.indexOf(QStringLiteral(":="));
        if (assignPos < 0) {
            continue;
        }
        const QString formal = part.left(assignPos).trimmed();
        const QString value = part.mid(assignPos + 2).trimmed();
        call->assignments.insert(formal, value);
    }
    return true;
}

bool parseAssignmentStatement(const QString &statement, QString *target, StExpr *rhs)
{
    static const QRegularExpression re(
        QStringLiteral("^\\s*([A-Za-z_]\\w*)\\s*:=\\s*(.+)\\s*;\\s*$"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = re.match(statement.trimmed());
    if (!match.hasMatch()) {
        return false;
    }
    *target = match.captured(1);
    StExprParser parser(match.captured(2).trimmed());
    *rhs = parser.parse();
    return rhs->type != StExpr::Empty;
}

bool parseGuardedAssignment(const QString &statement, CoilStatement *coilStatement)
{
    static const QRegularExpression re(
        QStringLiteral(
            "^\\s*IF\\s+(.+?)\\s+THEN\\s+([A-Za-z_]\\w*)\\s*:=\\s*(.+)\\s*;\\s*END_IF\\s*;\\s*$"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = re.match(statement.trimmed());
    if (!match.hasMatch()) {
        return false;
    }

    StExprParser guardParser(match.captured(1).trimmed());
    const StExpr guard = guardParser.parse();
    coilStatement->coilVariable = match.captured(2);

    const QString rhsText = match.captured(3).trimmed();
    if (rhsText.compare(QStringLiteral("TRUE"), Qt::CaseInsensitive) == 0) {
        coilStatement->coilKind = LadderCoilKind::Set;
        coilStatement->condition = guard;
        return true;
    }
    if (rhsText.compare(QStringLiteral("FALSE"), Qt::CaseInsensitive) == 0) {
        coilStatement->coilKind = LadderCoilKind::Reset;
        coilStatement->condition = guard;
        return true;
    }

    StExprParser rhsParser(rhsText);
    StExpr rhs = rhsParser.parse();
    if (rhs.type == StExpr::Ident) {
        StExpr combined;
        combined.type = StExpr::And;
        combined.args = {guard, rhs};
        coilStatement->condition = combined;
    } else if (rhs.type == StExpr::And) {
        StExpr combined;
        combined.type = StExpr::And;
        combined.args = {guard};
        combined.args.append(rhs.args);
        coilStatement->condition = combined;
    } else {
        StExpr combined;
        combined.type = StExpr::And;
        combined.args = {guard, rhs};
        coilStatement->condition = combined;
    }
    coilStatement->coilKind = LadderCoilKind::Normal;
    return true;
}

bool isBoolIdent(const QString &ident, const LadderProgram &program, const QMap<QString, FbInstance> &fbInstances)
{
    if (fbInstances.contains(ident)) {
        return false;
    }
    if (ident.contains(QLatin1Char('.'))) {
        const QString instance = ident.section(QLatin1Char('.'), 0, 0);
        return fbInstances.contains(instance);
    }
    for (const LadderVariable &var : program.variables) {
        if (var.name.compare(ident, Qt::CaseInsensitive) == 0) {
            return var.type == LadderVarType::Bool;
        }
    }
    return true;
}

void flattenAndAtoms(const StExpr &expr, const LadderProgram &program, const QMap<QString, FbInstance> &fbInstances,
                     QVector<StExpr> *atoms, QStringList *diagnostics)
{
    if (expr.type == StExpr::And) {
        for (const StExpr &arg : expr.args) {
            flattenAndAtoms(arg, program, fbInstances, atoms, diagnostics);
        }
        return;
    }
    if (expr.type == StExpr::TrueLit || expr.type == StExpr::FalseLit) {
        return;
    }
    if (expr.type == StExpr::Not) {
        atoms->push_back(expr);
        return;
    }
    if (expr.type == StExpr::Ident) {
        if (isBoolIdent(expr.ident, program, fbInstances)) {
            atoms->push_back(expr);
        } else {
            diagnostics->push_back(QStringLiteral("Non-boolean identifier skipped in condition: %1").arg(expr.ident));
        }
        return;
    }
    diagnostics->push_back(QStringLiteral("Unsupported expression node in AND branch."));
}

void addContactAtom(LadderProgram *program, int rungIndex, int row, int col, const StExpr &atom)
{
    if (atom.type == StExpr::Not && atom.args.size() == 1 && atom.args.first().type == StExpr::Ident) {
        program->addContact(rungIndex, row, col, atom.args.first().ident, true);
        return;
    }
    if (atom.type == StExpr::Ident) {
        if (atom.ident.contains(QLatin1Char('.'))) {
            return;
        }
        program->addContact(rungIndex, row, col, atom.ident, false);
    }
}

int addFunctionBlockCall(LadderProgram *program, int rungIndex, int row, int col, const FbCall &call,
                         const QMap<QString, FbInstance> &fbInstances)
{
    const FbInstance instance = fbInstances.value(call.instanceName);
    if (instance.typeName.isEmpty()) {
        return 0;
    }

    LadderElement &element = program->addFunctionBlock(rungIndex, row, col, instance.typeName);
    for (LadderPinBinding &binding : element.pinBindings) {
        const QString assigned = call.assignments.value(binding.formalName);
        if (assigned.isEmpty()) {
            continue;
        }
        static const QRegularExpression identRe(QStringLiteral("^[A-Za-z_]\\w*$"));
        if (identRe.match(assigned).hasMatch()) {
            binding.variable = assigned;
            binding.literal.clear();
            program->addVariableIfMissing(assigned);
        } else {
            binding.literal = assigned;
            binding.variable.clear();
        }
    }
    return element.localId;
}

void addBranchSeries(LadderProgram *program, int rungIndex, int row, int startCol, const StExpr &branch,
                     const QMap<QString, FbInstance> &fbInstances, QSet<QString> *placedFbInstances,
                     QStringList *diagnostics)
{
    QVector<StExpr> atoms;
    flattenAndAtoms(branch, *program, fbInstances, &atoms, diagnostics);

    int col = startCol;
    for (const StExpr &atom : atoms) {
        if (atom.type == StExpr::Ident && atom.ident.contains(QLatin1Char('.'))) {
            const QString instanceName = atom.ident.section(QLatin1Char('.'), 0, 0);
            if (!placedFbInstances->contains(instanceName)) {
                FbCall syntheticCall;
                syntheticCall.instanceName = instanceName;
                addFunctionBlockCall(program, rungIndex, row, col, syntheticCall, fbInstances);
                placedFbInstances->insert(instanceName);
            }
            ++col;
            continue;
        }
        addContactAtom(program, rungIndex, row, col, atom);
        ++col;
    }
}

void buildRungFromStatement(LadderProgram *program, const CoilStatement &statement,
                            const QMap<QString, FbInstance> &fbInstances, QStringList *diagnostics)
{
    const int rungIndex = program->rungs.size();
    program->addRung();
    program->addVariableIfMissing(statement.coilVariable);

    QSet<QString> placedFbInstances;
    int col = 1;
    for (const FbCall &call : statement.fbCalls) {
        addFunctionBlockCall(program, rungIndex, 0, col, call, fbInstances);
        placedFbInstances.insert(call.instanceName);
        ++col;
    }

    int branchRow = 0;
    if (statement.condition.type == StExpr::Or) {
        for (const StExpr &branch : statement.condition.args) {
            addBranchSeries(program, rungIndex, branchRow, 1, branch, fbInstances, &placedFbInstances, diagnostics);
            ++branchRow;
        }
    } else if (statement.condition.type != StExpr::Empty) {
        addBranchSeries(program, rungIndex, 0, col, statement.condition, fbInstances, &placedFbInstances, diagnostics);
    }

    int coilCol = 1;
    for (const LadderElement &element : program->rungs[rungIndex].elements) {
        if (element.kind != LadderElementKind::Coil) {
            coilCol = qMax(coilCol, element.col + 1);
        }
    }

    program->addCoil(rungIndex, 0, coilCol, statement.coilVariable, statement.coilKind);
    program->normalizeRungLayout(rungIndex);
    program->autoWireRung(rungIndex);
}

QVector<CoilStatement> parseBodyStatements(const QString &body, const LadderProgram &program,
                                           const QMap<QString, FbInstance> &fbInstances, QStringList *diagnostics)
{
    QVector<CoilStatement> rungStatements;
    QVector<FbCall> pendingFbCalls;

    const QStringList statements = splitTopLevelStatements(body);
    for (const QString &statement : statements) {
        FbCall fbCall;
        if (parseFbCall(statement, &fbCall)) {
            if (fbInstances.contains(fbCall.instanceName)) {
                pendingFbCalls.push_back(fbCall);
            } else {
                diagnostics->push_back(QStringLiteral("Ignored unknown function block call: %1").arg(fbCall.instanceName));
            }
            continue;
        }

        CoilStatement coilStatement;
        coilStatement.fbCalls = pendingFbCalls;
        pendingFbCalls.clear();

        if (parseGuardedAssignment(statement, &coilStatement)) {
            rungStatements.push_back(coilStatement);
            continue;
        }

        QString target;
        StExpr rhs;
        if (!parseAssignmentStatement(statement, &target, &rhs)) {
            diagnostics->push_back(QStringLiteral("Unsupported statement: %1").arg(statement.left(80)));
            continue;
        }

        if (rhs.type == StExpr::TrueLit || rhs.type == StExpr::FalseLit) {
            diagnostics->push_back(QStringLiteral("Non-ladder assignment skipped: %1").arg(statement.left(80)));
            continue;
        }

        coilStatement.coilVariable = target;
        coilStatement.condition = rhs;
        coilStatement.coilKind = LadderCoilKind::Normal;
        rungStatements.push_back(coilStatement);
    }

    if (!pendingFbCalls.isEmpty()) {
        diagnostics->push_back(QStringLiteral("Function block calls at end of program were ignored."));
    }

    Q_UNUSED(program);
    return rungStatements;
}

} // namespace

StLadderImporter::Result StLadderImporter::importSource(const QString &source)
{
    Result result;
    if (source.trimmed().isEmpty()) {
        result.diagnostics.push_back(QStringLiteral("Structured Text source is empty."));
        return result;
    }

    const QString cleaned = normalizeSpaces(stripComments(source));
    LadderProgram program;
    program.clear();

    QMap<QString, FbInstance> fbInstances;
    const QString varBlock = mergeVarBlocks(cleaned);
    if (!varBlock.isEmpty()) {
        parseVariables(varBlock, &program, &fbInstances, &result.diagnostics);
    }

    program.name = extractProgramName(cleaned);
    const QString body = extractProgramBody(cleaned);
    const QVector<CoilStatement> rungStatements = parseBodyStatements(body, program, fbInstances, &result.diagnostics);

    if (rungStatements.isEmpty()) {
        result.diagnostics.push_back(QStringLiteral("No ladder-compatible coil assignments were found in the ST source."));
        return result;
    }

    for (const CoilStatement &statement : rungStatements) {
        buildRungFromStatement(&program, statement, fbInstances, &result.diagnostics);
    }

    program.ensureBoardIoVariables();
    program.normalizeAllRungs();
    for (int i = 0; i < program.rungs.size(); ++i) {
        program.autoWireRung(i);
    }

    const QVector<LadderValidator::Issue> issues = LadderValidator::validate(program);
    for (const LadderValidator::Issue &issue : issues) {
        if (issue.severity == LadderValidator::Issue::Severity::Warning) {
            result.diagnostics.push_back(issue.message);
        } else if (issue.severity == LadderValidator::Issue::Severity::Error) {
            result.diagnostics.push_back(QStringLiteral("Validation: %1").arg(issue.message));
        }
    }

    result.program = program;
    result.ok = !program.rungs.isEmpty();
    return result;
}
