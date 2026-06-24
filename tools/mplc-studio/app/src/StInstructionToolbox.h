/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QList>
#include <QVBoxLayout>
#include <QWidget>

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QScrollArea;
class QLabel;
class QToolButton;

class StInstructionToolbox : public QWidget
{
    Q_OBJECT

public:
    explicit StInstructionToolbox(QPlainTextEdit *editor, QWidget *parent = nullptr);

    void setExpanded(bool expanded);
    bool isExpanded() const;

signals:
    void expandedChanged(bool expanded);

private:
    struct SnippetEntry {
        QWidget *row = nullptr;
        QWidget *groupBody = nullptr;
        QString searchText;
    };

    struct ToolboxGroup {
        QToolButton *header = nullptr;
        QWidget *body = nullptr;
    };

    void insertSnippet(const QString &snippet);
    QWidget *addCollapsibleGroup(QWidget *parent,
                                 QVBoxLayout *layout,
                                 const QString &title,
                                 const QList<std::pair<QString, QString>> &items);
    QWidget *makeSnippetRow(QWidget *parent, const QString &label, const QString &snippet);
    void filterSnippets(const QString &query);
    void onGroupToggled(QToolButton *source, bool expanded);

    QPlainTextEdit *m_editor = nullptr;
    QWidget *m_expandedPanel = nullptr;
    QWidget *m_collapsedTab = nullptr;
    QWidget *m_panel = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_subtitleLabel = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QPushButton *m_toggleButton = nullptr;
    QList<SnippetEntry> m_snippetEntries;
    QList<ToolboxGroup> m_groups;
    bool m_expanded = true;
};
