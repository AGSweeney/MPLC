/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class QTextDocument;

class StSyntaxHighlighter : public QSyntaxHighlighter
{
public:
    explicit StSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    void applyRules(const QString &text, int start, int length);

    QVector<HighlightRule> m_rules;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_typeFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_addressFormat;
    QTextCharFormat m_operatorFormat;
    QTextCharFormat m_constantFormat;

    enum BlockState {
        Normal = 0,
        InBlockComment = 1,
    };
};
