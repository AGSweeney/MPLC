/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderPlcopenSerializer.h"
#include "LadderBranchLogic.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace {

QString coilActionString(LadderCoilKind kind)
{
    switch (kind) {
    case LadderCoilKind::Set:
        return QStringLiteral("set");
    case LadderCoilKind::Reset:
        return QStringLiteral("reset");
    case LadderCoilKind::Normal:
    default:
        return QString();
    }
}

LadderCoilKind coilKindFromAction(const QString &action)
{
    if (action.compare(QStringLiteral("set"), Qt::CaseInsensitive) == 0) {
        return LadderCoilKind::Set;
    }
    if (action.compare(QStringLiteral("reset"), Qt::CaseInsensitive) == 0) {
        return LadderCoilKind::Reset;
    }
    return LadderCoilKind::Normal;
}

int nextIdForProgram(LadderProgram &program, int candidate)
{
    if (candidate >= program.nextLocalId) {
        program.nextLocalId = candidate + 1;
    }
    return candidate;
}

} // namespace

bool LadderPlcopenSerializer::load(const QString &path, LadderProgram &program, QString *errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    program.clear();

    QXmlStreamReader xml(&file);
    LadderRung *currentRung = nullptr;
    LadderElement *currentBlock = nullptr;
    int rungCounter = 0;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const QString name = xml.name().toString();
            if (name == QStringLiteral("contentHeader")) {
                const QString projectName = xml.attributes().value(QStringLiteral("name")).toString();
                if (!projectName.isEmpty()) {
                    program.name = projectName;
                }
            } else if (name == QStringLiteral("rung")) {
                currentRung = &program.addRung();
                const int localId = xml.attributes().value(QStringLiteral("localId")).toInt();
                if (localId > 0) {
                    currentRung->localId = nextIdForProgram(program, localId);
                }
                currentRung->branchForkCol = xml.attributes().value(QStringLiteral("branchForkCol")).toInt();
                currentRung->branchJoinCol = xml.attributes().value(QStringLiteral("branchJoinCol")).toInt();
                currentRung->branchRowsBelow = xml.attributes().value(QStringLiteral("branchRowsBelow")).toInt();
                currentRung->outputBranchForkCol =
                    xml.attributes().value(QStringLiteral("outputBranchForkCol")).toInt();
                rungCounter++;
                Q_UNUSED(rungCounter);
            } else if (name == QStringLiteral("inputBranch") && currentRung != nullptr) {
                LadderInputBranchRegion region;
                region.forkCol = xml.attributes().value(QStringLiteral("forkCol")).toInt();
                region.joinCol = xml.attributes().value(QStringLiteral("joinCol")).toInt();
                region.pathRow = xml.attributes().value(QStringLiteral("pathRow")).toInt();
                if (region.forkCol > 0 && region.joinCol > region.forkCol) {
                    currentRung->inputBranchRegions.push_back(region);
                }
            } else if (name == QStringLiteral("contact")) {
                if (currentRung == nullptr) {
                    currentRung = &program.addRung();
                }
                LadderElement element;
                element.localId = nextIdForProgram(program,
                                                   xml.attributes().value(QStringLiteral("localId")).toInt());
                if (element.localId <= 0) {
                    element.localId = program.allocateLocalId();
                }
                element.kind = LadderElementKind::Contact;
                element.rungIndex = program.rungs.size() - 1;
                element.variable = xml.attributes().value(QStringLiteral("variable")).toString();
                element.negated = xml.attributes().value(QStringLiteral("negated")).compare(QStringLiteral("true"),
                                                                                             Qt::CaseInsensitive) == 0;
                element.row = xml.attributes().value(QStringLiteral("row")).toInt();
                element.col = xml.attributes().value(QStringLiteral("col")).toInt();
                currentRung->elements.push_back(element);
                program.addVariableIfMissing(element.variable);
            } else if (name == QStringLiteral("coil")) {
                if (currentRung == nullptr) {
                    currentRung = &program.addRung();
                }
                LadderElement element;
                element.localId = nextIdForProgram(program,
                                                   xml.attributes().value(QStringLiteral("localId")).toInt());
                if (element.localId <= 0) {
                    element.localId = program.allocateLocalId();
                }
                element.kind = LadderElementKind::Coil;
                element.rungIndex = program.rungs.size() - 1;
                element.variable = xml.attributes().value(QStringLiteral("variable")).toString();
                element.coilKind = coilKindFromAction(xml.attributes().value(QStringLiteral("coilAction")).toString());
                element.row = xml.attributes().value(QStringLiteral("row")).toInt();
                element.col = xml.attributes().value(QStringLiteral("col")).toInt();
                currentRung->elements.push_back(element);
                program.addVariableIfMissing(element.variable);
            } else if (name == QStringLiteral("block")) {
                if (currentRung == nullptr) {
                    currentRung = &program.addRung();
                }
                LadderElement element;
                element.localId = nextIdForProgram(program,
                                                   xml.attributes().value(QStringLiteral("localId")).toInt());
                if (element.localId <= 0) {
                    element.localId = program.allocateLocalId();
                }
                element.kind = LadderElementKind::FunctionBlock;
                element.rungIndex = program.rungs.size() - 1;
                element.fbTypeName = xml.attributes().value(QStringLiteral("typeName")).toString();
                element.row = xml.attributes().value(QStringLiteral("row")).toInt();
                element.col = xml.attributes().value(QStringLiteral("col")).toInt();
                currentRung->elements.push_back(element);
                currentBlock = &currentRung->elements.last();
            } else if (name == QStringLiteral("connection") && currentRung != nullptr) {
                LadderConnection connection;
                connection.fromLocalId = xml.attributes().value(QStringLiteral("fromLocalId")).toInt();
                connection.toLocalId = xml.attributes().value(QStringLiteral("toLocalId")).toInt();
                connection.fromPin = xml.attributes().value(QStringLiteral("fromPin")).toString();
                connection.toPin = xml.attributes().value(QStringLiteral("toPin")).toString();
                currentRung->connections.push_back(connection);
            } else if (name == QStringLiteral("variable") && currentBlock != nullptr) {
                LadderPinBinding binding;
                binding.formalName = xml.attributes().value(QStringLiteral("formalParameter")).toString();
                binding.variable = xml.attributes().value(QStringLiteral("variable")).toString();
                binding.literal = xml.attributes().value(QStringLiteral("literal")).toString();
                currentBlock->pinBindings.push_back(binding);
                if (!binding.variable.isEmpty()) {
                    program.addVariableIfMissing(binding.variable);
                }
            } else if (name == QStringLiteral("varDeclaration")) {
                LadderVariable var;
                var.name = xml.attributes().value(QStringLiteral("name")).toString();
                const QString typeName = xml.attributes().value(QStringLiteral("type")).toString();
                if (typeName.compare(QStringLiteral("TIME"), Qt::CaseInsensitive) == 0) {
                    var.type = LadderVarType::Time;
                } else if (typeName.compare(QStringLiteral("INT"), Qt::CaseInsensitive) == 0) {
                    var.type = LadderVarType::Int;
                } else {
                    var.type = LadderVarType::Bool;
                }
                var.ioAddress = xml.attributes().value(QStringLiteral("address")).toString();
                if (!var.name.isEmpty()) {
                    program.variables.push_back(var);
                }
            }
        } else if (xml.isEndElement()) {
            if (xml.name() == QStringLiteral("block")) {
                currentBlock = nullptr;
            } else if (xml.name() == QStringLiteral("rung")) {
                currentRung = nullptr;
            }
        }
    }

    if (xml.hasError()) {
        if (errorMessage != nullptr) {
            *errorMessage = xml.errorString();
        }
        return false;
    }

    if (program.rungs.isEmpty()) {
        program.ensureBoardIoVariables();
    } else {
        program.normalizeAllRungs();
        for (LadderRung &rung : program.rungs) {
            rung.ensureInputBranchRegions();
        }
        for (int i = 0; i < program.rungs.size(); ++i) {
            program.autoWireRung(i);
        }
    }

    program.ensureBoardIoVariables();
    return true;
}

bool LadderPlcopenSerializer::save(const QString &path, const LadderProgram &program, QString *errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(QStringLiteral("project"));
    xml.writeAttribute(QStringLiteral("xmlns"), QStringLiteral("http://www.plcopen.org/xml/tc6_0200"));

    xml.writeStartElement(QStringLiteral("types"));
    xml.writeStartElement(QStringLiteral("dataTypes"));
    xml.writeEndElement();
    xml.writeStartElement(QStringLiteral("pous"));
    xml.writeEndElement();
    xml.writeEndElement();

    xml.writeStartElement(QStringLiteral("instances"));
    xml.writeStartElement(QStringLiteral("configurations"));
    xml.writeEndElement();
    xml.writeEndElement();

    xml.writeStartElement(QStringLiteral("contentHeader"));
    xml.writeAttribute(QStringLiteral("name"), program.name);
    xml.writeEndElement();

    xml.writeStartElement(QStringLiteral("body"));
    xml.writeStartElement(QStringLiteral("LD"));

    xml.writeStartElement(QStringLiteral("leftPowerRail"));
    xml.writeEmptyElement(QStringLiteral("connectionPointOut"));
    xml.writeEndElement();

    for (const LadderVariable &var : program.variables) {
        xml.writeStartElement(QStringLiteral("varDeclaration"));
        xml.writeAttribute(QStringLiteral("name"), var.name);
        switch (var.type) {
        case LadderVarType::Int:
            xml.writeAttribute(QStringLiteral("type"), QStringLiteral("INT"));
            break;
        case LadderVarType::Time:
            xml.writeAttribute(QStringLiteral("type"), QStringLiteral("TIME"));
            break;
        case LadderVarType::Bool:
        default:
            xml.writeAttribute(QStringLiteral("type"), QStringLiteral("BOOL"));
            break;
        }
        if (!var.ioAddress.isEmpty()) {
            xml.writeAttribute(QStringLiteral("address"), var.ioAddress);
        }
        xml.writeEndElement();
    }

    for (const LadderRung &rung : program.rungs) {
        xml.writeStartElement(QStringLiteral("rung"));
        xml.writeAttribute(QStringLiteral("localId"), QString::number(rung.localId));
        if (rung.branchForkCol > 0 && rung.branchJoinCol > rung.branchForkCol) {
            xml.writeAttribute(QStringLiteral("branchForkCol"), QString::number(rung.branchForkCol));
            xml.writeAttribute(QStringLiteral("branchJoinCol"), QString::number(rung.branchJoinCol));
            if (rung.branchRowsBelow > 0) {
                xml.writeAttribute(QStringLiteral("branchRowsBelow"), QString::number(rung.branchRowsBelow));
            }
        }
        if (rung.outputBranchForkCol > 0) {
            xml.writeAttribute(QStringLiteral("outputBranchForkCol"), QString::number(rung.outputBranchForkCol));
        }

        const QVector<LadderInputBranchRegion> inputRegions = LadderBranchLogic::inputBranchRegions(rung);
        for (const LadderInputBranchRegion &region : inputRegions) {
            xml.writeEmptyElement(QStringLiteral("inputBranch"));
            xml.writeAttribute(QStringLiteral("forkCol"), QString::number(region.forkCol));
            xml.writeAttribute(QStringLiteral("joinCol"), QString::number(region.joinCol));
            xml.writeAttribute(QStringLiteral("pathRow"), QString::number(region.pathRow));
        }

        for (const LadderElement &element : rung.elements) {
            if (element.kind == LadderElementKind::Contact) {
                xml.writeStartElement(QStringLiteral("contact"));
                xml.writeAttribute(QStringLiteral("localId"), QString::number(element.localId));
                xml.writeAttribute(QStringLiteral("variable"), element.variable);
                if (element.negated) {
                    xml.writeAttribute(QStringLiteral("negated"), QStringLiteral("true"));
                }
                xml.writeAttribute(QStringLiteral("row"), QString::number(element.row));
                xml.writeAttribute(QStringLiteral("col"), QString::number(element.col));
                xml.writeEndElement();
            } else if (element.kind == LadderElementKind::Coil) {
                xml.writeStartElement(QStringLiteral("coil"));
                xml.writeAttribute(QStringLiteral("localId"), QString::number(element.localId));
                xml.writeAttribute(QStringLiteral("variable"), element.variable);
                const QString action = coilActionString(element.coilKind);
                if (!action.isEmpty()) {
                    xml.writeAttribute(QStringLiteral("coilAction"), action);
                }
                xml.writeAttribute(QStringLiteral("row"), QString::number(element.row));
                xml.writeAttribute(QStringLiteral("col"), QString::number(element.col));
                xml.writeEndElement();
            } else if (element.kind == LadderElementKind::FunctionBlock) {
                xml.writeStartElement(QStringLiteral("block"));
                xml.writeAttribute(QStringLiteral("typeName"), element.fbTypeName);
                xml.writeAttribute(QStringLiteral("localId"), QString::number(element.localId));
                xml.writeAttribute(QStringLiteral("row"), QString::number(element.row));
                xml.writeAttribute(QStringLiteral("col"), QString::number(element.col));
                for (const LadderPinBinding &pin : element.pinBindings) {
                    xml.writeStartElement(QStringLiteral("variable"));
                    xml.writeAttribute(QStringLiteral("formalParameter"), pin.formalName);
                    if (!pin.variable.isEmpty()) {
                        xml.writeAttribute(QStringLiteral("variable"), pin.variable);
                    }
                    if (!pin.literal.isEmpty()) {
                        xml.writeAttribute(QStringLiteral("literal"), pin.literal);
                    }
                    xml.writeEndElement();
                }
                xml.writeEndElement();
            }
        }

        for (const LadderConnection &connection : rung.connections) {
            xml.writeStartElement(QStringLiteral("connection"));
            xml.writeAttribute(QStringLiteral("fromLocalId"), QString::number(connection.fromLocalId));
            xml.writeAttribute(QStringLiteral("toLocalId"), QString::number(connection.toLocalId));
            if (!connection.fromPin.isEmpty()) {
                xml.writeAttribute(QStringLiteral("fromPin"), connection.fromPin);
            }
            if (!connection.toPin.isEmpty()) {
                xml.writeAttribute(QStringLiteral("toPin"), connection.toPin);
            }
            xml.writeEndElement();
        }

        xml.writeEndElement();
    }

    xml.writeEndElement(); // LD
    xml.writeEndElement(); // body
    xml.writeEndElement(); // project
    xml.writeEndDocument();
    return true;
}
