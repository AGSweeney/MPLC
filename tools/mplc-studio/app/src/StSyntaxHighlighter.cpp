/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "StSyntaxHighlighter.h"

#include <QColor>
#include <QFont>
#include <QTextDocument>

StSyntaxHighlighter::StSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    m_keywordFormat.setForeground(QColor("#0000FF"));
    m_keywordFormat.setFontWeight(QFont::Bold);

    m_typeFormat.setForeground(QColor("#2B91AF"));
    m_typeFormat.setFontWeight(QFont::Bold);

    m_commentFormat.setForeground(QColor("#008000"));
    m_commentFormat.setFontItalic(true);

    m_numberFormat.setForeground(QColor("#098658"));

    m_addressFormat.setForeground(QColor("#811F3F"));

    m_operatorFormat.setForeground(QColor("#AF00DB"));
    m_operatorFormat.setFontWeight(QFont::Bold);

    m_constantFormat.setForeground(QColor("#0000FF"));
    m_constantFormat.setFontWeight(QFont::Bold);

    const QStringList keywords = {
        "PROGRAM", "END_PROGRAM",
        "VAR", "END_VAR", "VAR_INPUT", "END_VAR_INPUT",
        "VAR_OUTPUT", "END_VAR_OUTPUT", "VAR_GLOBAL", "END_VAR_GLOBAL",
        "VAR_TEMP", "VAR_EXTERNAL", "VAR_ACCESS", "VAR_CONFIG",
        "CONSTANT", "RETAIN", "AT",
        "IF", "THEN", "ELSE", "ELSIF", "END_IF",
        "CASE", "OF", "END_CASE",
        "FOR", "TO", "BY", "DO", "END_FOR",
        "WHILE", "END_WHILE",
        "REPEAT", "UNTIL", "END_REPEAT",
        "RETURN", "EXIT", "CONTINUE",
        "AND", "OR", "XOR", "NOT", "MOD",
        "FUNCTION", "END_FUNCTION",
        "FUNCTION_BLOCK", "END_FUNCTION_BLOCK",
    };

    const QStringList types = {
        "BOOL", "BYTE", "WORD", "DWORD", "LWORD",
        "SINT", "INT", "DINT", "LINT",
        "USINT", "UINT", "UDINT", "ULINT",
        "REAL", "LREAL",
        "TIME", "DATE", "TOD", "DT", "DATE_AND_TIME",
        "STRING", "WSTRING",
    };

    const QStringList constants = {
        "TRUE", "FALSE",
    };

    for (const QString &word : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(
            QString("\\b%1\\b").arg(word),
            QRegularExpression::CaseInsensitiveOption);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    for (const QString &word : types) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(
            QString("\\b%1\\b").arg(word),
            QRegularExpression::CaseInsensitiveOption);
        rule.format = m_typeFormat;
        m_rules.append(rule);
    }

    for (const QString &word : constants) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(
            QString("\\b%1\\b").arg(word),
            QRegularExpression::CaseInsensitiveOption);
        rule.format = m_constantFormat;
        m_rules.append(rule);
    }

    HighlightRule numberRule;
    numberRule.pattern = QRegularExpression("\\b\\d+\\b");
    numberRule.format = m_numberFormat;
    m_rules.append(numberRule);

    HighlightRule addressRule;
    addressRule.pattern = QRegularExpression(
        "%[IQMT][XBWDL][\\d.]+",
        QRegularExpression::CaseInsensitiveOption);
    addressRule.format = m_addressFormat;
    m_rules.append(addressRule);

    HighlightRule assignRule;
    assignRule.pattern = QRegularExpression(":=");
    assignRule.format = m_operatorFormat;
    m_rules.append(assignRule);
}

void StSyntaxHighlighter::applyRules(const QString &text, int start, int length)
{
    if (length <= 0) {
        return;
    }

    const QString segment = text.mid(start, length);
    for (const HighlightRule &rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(segment);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            setFormat(start + match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

void StSyntaxHighlighter::highlightBlock(const QString &text)
{
    setCurrentBlockState(Normal);

    int scanStart = 0;

    if (previousBlockState() == InBlockComment) {
        const int endIndex = text.indexOf("*)");
        if (endIndex == -1) {
            setFormat(0, text.length(), m_commentFormat);
            setCurrentBlockState(InBlockComment);
            return;
        }
        setFormat(0, endIndex + 2, m_commentFormat);
        scanStart = endIndex + 2;
    }

    while (scanStart < text.length()) {
        const int lineComment = text.indexOf("//", scanStart);
        const int blockStart = text.indexOf("(*", scanStart);

        int nextSpecial = text.length();
        enum SpecialKind { None, LineComment, BlockComment } kind = None;

        if (lineComment >= 0 && lineComment < nextSpecial) {
            nextSpecial = lineComment;
            kind = LineComment;
        }
        if (blockStart >= 0 && blockStart < nextSpecial) {
            nextSpecial = blockStart;
            kind = BlockComment;
        }

        if (kind == None) {
            applyRules(text, scanStart, text.length() - scanStart);
            return;
        }

        if (nextSpecial > scanStart) {
            applyRules(text, scanStart, nextSpecial - scanStart);
        }

        if (kind == LineComment) {
            setFormat(nextSpecial, text.length() - nextSpecial, m_commentFormat);
            return;
        }

        const int blockEnd = text.indexOf("*)", nextSpecial + 2);
        if (blockEnd == -1) {
            setFormat(nextSpecial, text.length() - nextSpecial, m_commentFormat);
            setCurrentBlockState(InBlockComment);
            return;
        }
        setFormat(nextSpecial, blockEnd - nextSpecial + 2, m_commentFormat);
        scanStart = blockEnd + 2;
    }
}
