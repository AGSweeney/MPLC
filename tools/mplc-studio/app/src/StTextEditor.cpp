/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "StTextEditor.h"
#include "StSyntaxHighlighter.h"

#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QPlainTextEdit>
#include <QVBoxLayout>

StTextEditor::StTextEditor(QWidget *parent)
    : DocumentEditor(Kind::StructuredText, parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_editor = new QPlainTextEdit(this);
    m_editor->setObjectName("CodeEditor");
    m_editor->setPlaceholderText("Write Structured Text here...");
    m_editor->setTabStopDistance(48);
    m_editor->setFont(QFont(QStringLiteral("Consolas"), 13));
    new StSyntaxHighlighter(m_editor->document());

    layout->addWidget(m_editor);

    connect(m_editor, &QPlainTextEdit::textChanged, this, [this]() {
        setDirty(true);
    });
}

bool StTextEditor::loadFromPath(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    m_editor->setPlainText(QString::fromUtf8(file.readAll()));
    m_filePath = QFileInfo(path).absoluteFilePath();
    setDirty(false);
    return true;
}

bool StTextEditor::saveToPath(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    if (file.write(m_editor->toPlainText().toUtf8()) < 0) {
        return false;
    }

    m_filePath = QFileInfo(path).absoluteFilePath();
    setDirty(false);
    return true;
}

void StTextEditor::markClean()
{
    setDirty(false);
}

void StTextEditor::newDocument()
{
    m_editor->setPlainText(
        "PROGRAM Main\n"
        "VAR\n"
        "    Led1 AT %QX0.0 : BOOL;\n"
        "END_VAR\n"
        "    Led1 := TRUE;\n"
        "END_PROGRAM\n");
    m_filePath.clear();
    setDirty(true);
}

QString StTextEditor::suggestedSaveFilter() const
{
    return QStringLiteral("Structured Text (*.st)");
}

QString StTextEditor::suggestedOpenFilter() const
{
    return QStringLiteral("Structured Text (*.st)");
}

QString StTextEditor::compileInputPath(QString *tempCleanupPath)
{
    if (tempCleanupPath != nullptr) {
        tempCleanupPath->clear();
    }
    return m_filePath;
}
