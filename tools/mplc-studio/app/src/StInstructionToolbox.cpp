/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "StInstructionToolbox.h"
#include "StudioTheme.h"

#include <QFrame>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QSizePolicy>
#include <QTextBlock>
#include <QTextCursor>
#include <QToolButton>
#include <QVBoxLayout>
#include <functional>

namespace {

constexpr int kChevronSize = 12;
constexpr int kCollapsedTabWidth = 24;

QIcon toolboxChevronIcon(bool expanded)
{
    const QString path = expanded ? QStringLiteral(":/icons/chevron-down.svg")
                                : QStringLiteral(":/icons/chevron-right.svg");
    return StudioTheme::renderSvgIcon(path, kChevronSize, QColor("#2d325a"));
}

class ToolboxCollapsedTab final : public QWidget
{
public:
    explicit ToolboxCollapsedTab(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setObjectName("StToolboxCollapsedTab");
        setFixedWidth(kCollapsedTabWidth);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        setCursor(Qt::PointingHandCursor);
        setToolTip("Show Toolbox");
    }

    std::function<void()> onActivate;

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QWidget::paintEvent(event);

        QPainter painter(this);
        painter.setPen(QColor("#2d325a"));
        QFont labelFont = painter.font();
        labelFont.setPointSize(9);
        labelFont.setBold(true);
        painter.setFont(labelFont);

        painter.translate(width(), 0);
        painter.rotate(90);
        painter.drawText(QRect(0, 0, height(), width()), Qt::AlignCenter, QStringLiteral("Toolbox"));
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && onActivate) {
            onActivate();
        }
        QWidget::mousePressEvent(event);
    }
};

} // namespace

StInstructionToolbox::StInstructionToolbox(QPlainTextEdit *editor, QWidget *parent)
    : QWidget(parent)
    , m_editor(editor)
{
    setObjectName("StToolbox");
    setMinimumWidth(kCollapsedTabWidth);
    setMaximumWidth(300);

    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_expandedPanel = new QWidget(this);
    m_expandedPanel->setObjectName("StToolboxExpandedPanel");
    auto *expandedLayout = new QVBoxLayout(m_expandedPanel);
    expandedLayout->setContentsMargins(0, 0, 0, 0);
    expandedLayout->setSpacing(0);

    auto *header = new QWidget(this);
    header->setObjectName("StToolboxHeader");
    header->setFixedHeight(40);
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(10, 6, 6, 6);
    headerLayout->setSpacing(0);

    auto *titleRow = new QHBoxLayout();
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(4);

    auto *title = new QLabel("Toolbox", header);
    title->setObjectName("StToolboxTitle");
    m_titleLabel = title;

    m_toggleButton = new QPushButton(header);
    m_toggleButton->setObjectName("StToolboxToggle");
    m_toggleButton->setFixedSize(22, 22);
    m_toggleButton->setFlat(true);
    m_toggleButton->setCursor(Qt::PointingHandCursor);
    m_toggleButton->setIcon(StudioTheme::renderSvgIcon(QStringLiteral(":/icons/chevron-right.svg"),
                                                         10,
                                                         QColor("#ffffff")));
    m_toggleButton->setToolTip("Hide Toolbox");

    titleRow->addWidget(title, 1);
    titleRow->addWidget(m_toggleButton);

    auto *subtitle = new QLabel("Structured Text snippets", header);
    subtitle->setObjectName("StToolboxSubtitle");
    m_subtitleLabel = subtitle;

    headerLayout->addLayout(titleRow);
    headerLayout->addWidget(subtitle);

    auto *searchBar = new QWidget(this);
    searchBar->setObjectName("StToolboxSearchBar");
    searchBar->setFixedHeight(34);
    auto *searchLayout = new QHBoxLayout(searchBar);
    searchLayout->setContentsMargins(8, 4, 8, 4);
    searchLayout->setSpacing(6);

    auto *searchIcon = new QLabel(searchBar);
    searchIcon->setObjectName("StToolboxSearchIcon");
    searchIcon->setPixmap(StudioTheme::renderSvgIcon(QStringLiteral(":/icons/search.svg"),
                                                     14,
                                                     QColor("#5a5a5a"))
                              .pixmap(14, 14));
    searchIcon->setFixedSize(16, 16);

    m_searchEdit = new QLineEdit(searchBar);
    m_searchEdit->setObjectName("StToolboxSearch");
    m_searchEdit->setPlaceholderText("Filter instructions…");
    m_searchEdit->setClearButtonEnabled(true);

    searchLayout->addWidget(searchIcon);
    searchLayout->addWidget(m_searchEdit, 1);

    m_panel = new QWidget(this);
    m_panel->setObjectName("StToolboxPanel");
    auto *panelLayout = new QVBoxLayout(m_panel);
    panelLayout->setContentsMargins(6, 4, 6, 8);
    panelLayout->setSpacing(6);

    addCollapsibleGroup(m_panel, panelLayout, "Structure", {
        {"VAR block", "VAR\n    Name : DINT;\nEND_VAR\n"},
        {"PROGRAM", "PROGRAM Main\n\nEND_PROGRAM\n"},
    });

    addCollapsibleGroup(m_panel, panelLayout, "Variables", {
        {"BOOL local", "MyFlag : BOOL;\n"},
        {"DINT local", "Counter : DINT;\n"},
        {"BOOL input", "Input1 AT %IX0.0 : BOOL;\n"},
        {"BOOL output", "Output1 AT %QX0.0 : BOOL;\n"},
    });

    addCollapsibleGroup(m_panel, panelLayout, "Control", {
        {"IF / THEN", "IF condition THEN\n    \nEND_IF;\n"},
        {"IF / ELSE", "IF condition THEN\n    \nELSE\n    \nEND_IF;\n"},
        {"IF / ELSIF", "IF condition THEN\n    \nELSIF other THEN\n    \nELSE\n    \nEND_IF;\n"},
        {"WHILE", "WHILE condition DO\n    \nEND_WHILE;\n"},
        {"REPEAT", "REPEAT\n    \nUNTIL condition;\nEND_REPEAT;\n"},
        {"FOR", "FOR i := 1 TO 10 BY 1 DO\n    \nEND_FOR;\n"},
    });

    addCollapsibleGroup(m_panel, panelLayout, "Logic & math", {
        {"Assignment", "Target := value;\n"},
        {"AND / OR", "IF (A AND B) OR C THEN\n    \nEND_IF;\n"},
        {"NOT", "IF NOT Flag THEN\n    \nEND_IF;\n"},
        {"XOR", "Result := A XOR B;\n"},
        {"MOD", "Result := Value MOD 10;\n"},
        {"Compare", "IF Value >= 0 THEN\n    \nEND_IF;\n"},
    });

    addCollapsibleGroup(m_panel, panelLayout, "Comments", {
        {"Line comment", "// comment\n"},
        {"Block comment", "(* comment *)\n"},
    });

    panelLayout->addStretch(1);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("StToolboxScroll");
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidget(m_panel);

    expandedLayout->addWidget(header);
    expandedLayout->addWidget(searchBar);
    expandedLayout->addWidget(m_scrollArea, 1);

    m_collapsedTab = new ToolboxCollapsedTab(this);
    static_cast<ToolboxCollapsedTab *>(m_collapsedTab)->onActivate = [this]() {
        setExpanded(true);
    };
    m_collapsedTab->setVisible(false);

    rootLayout->addWidget(m_expandedPanel, 1);
    rootLayout->addWidget(m_collapsedTab);

    connect(m_toggleButton, &QPushButton::clicked, this, [this]() {
        setExpanded(!m_expanded);
    });
    connect(m_searchEdit, &QLineEdit::textChanged, this, &StInstructionToolbox::filterSnippets);
}

void StInstructionToolbox::setExpanded(bool expanded)
{
    if (m_expanded == expanded) {
        return;
    }
    m_expanded = expanded;

    if (m_expandedPanel != nullptr) {
        m_expandedPanel->setVisible(expanded);
    }
    if (m_collapsedTab != nullptr) {
        m_collapsedTab->setVisible(!expanded);
    }
    if (m_toggleButton != nullptr) {
        m_toggleButton->setIcon(StudioTheme::renderSvgIcon(QStringLiteral(":/icons/chevron-right.svg"),
                                                           10,
                                                           QColor("#ffffff")));
        m_toggleButton->setToolTip(expanded ? "Hide Toolbox" : "Show Toolbox");
    }
    setMaximumWidth(expanded ? 300 : kCollapsedTabWidth);
    if (expanded) {
        setMinimumWidth(220);
    } else {
        setMinimumWidth(kCollapsedTabWidth);
    }
    emit expandedChanged(expanded);
}

bool StInstructionToolbox::isExpanded() const
{
    return m_expanded;
}

void StInstructionToolbox::insertSnippet(const QString &snippet)
{
    if (m_editor == nullptr || snippet.isEmpty()) {
        return;
    }

    QTextCursor cursor = m_editor->textCursor();
    if (!cursor.hasSelection()) {
        const int lineStart = cursor.block().position();
        const QString lineText = cursor.block().text();
        if (lineText.trimmed().isEmpty() && cursor.position() == lineStart) {
            cursor.movePosition(QTextCursor::StartOfBlock);
        }
    }
    cursor.insertText(snippet);
    m_editor->setTextCursor(cursor);
    m_editor->setFocus();
}

QWidget *StInstructionToolbox::makeSnippetRow(QWidget *parent, const QString &label, const QString &snippet)
{
    auto *row = new QPushButton(label, parent);
    row->setObjectName("StToolboxSnippet");
    row->setFlat(true);
    row->setCursor(Qt::PointingHandCursor);
    row->setToolTip(snippet.trimmed());
    row->setMinimumHeight(40);
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    connect(row, &QPushButton::clicked, this, [this, snippet]() {
        insertSnippet(snippet);
    });

    return row;
}

QWidget *StInstructionToolbox::addCollapsibleGroup(QWidget *parent,
                                                   QVBoxLayout *layout,
                                                   const QString &title,
                                                   const QList<std::pair<QString, QString>> &items)
{
    auto *group = new QWidget(parent);
    group->setObjectName("StToolboxGroup");
    auto *groupLayout = new QVBoxLayout(group);
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(0);

    auto *header = new QToolButton(group);
    header->setObjectName("StToolboxGroupHeader");
    header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    header->setIcon(toolboxChevronIcon(true));
    header->setText(title);
    header->setCheckable(true);
    header->setChecked(false);
    header->setAutoRaise(false);
    header->setCursor(Qt::PointingHandCursor);
    header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    header->setIcon(toolboxChevronIcon(false));

    auto *body = new QWidget(group);
    body->setObjectName("StToolboxGroupBody");
    body->setVisible(false);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(6, 4, 6, 6);
    bodyLayout->setSpacing(4);

    for (const auto &item : items) {
        QWidget *row = makeSnippetRow(body, item.first, item.second);
        bodyLayout->addWidget(row);

        SnippetEntry entry;
        entry.row = row;
        entry.groupBody = body;
        entry.searchText = QString("%1 %2 %3").arg(title, item.first, item.second).toLower();
        m_snippetEntries.append(entry);
    }

    groupLayout->addWidget(header);
    groupLayout->addWidget(body);

    ToolboxGroup toolboxGroup;
    toolboxGroup.header = header;
    toolboxGroup.body = body;
    m_groups.append(toolboxGroup);

    connect(header, &QToolButton::toggled, this, [this, header](bool expanded) {
        onGroupToggled(header, expanded);
    });

    layout->addWidget(group);
    return body;
}

void StInstructionToolbox::onGroupToggled(QToolButton *source, bool expanded)
{
    if (source == nullptr) {
        return;
    }

    for (ToolboxGroup &group : m_groups) {
        if (group.header == source) {
            group.body->setVisible(expanded);
            source->setIcon(toolboxChevronIcon(expanded));
            if (expanded) {
                for (ToolboxGroup &other : m_groups) {
                    if (other.header != source && other.header->isChecked()) {
                        other.header->setChecked(false);
                    }
                }
            }
            return;
        }
    }
}

void StInstructionToolbox::filterSnippets(const QString &query)
{
    const QString needle = query.trimmed().toLower();
    QSet<QWidget *> groupsWithMatches;
    QSet<QWidget *> allGroups;

    for (const SnippetEntry &entry : m_snippetEntries) {
        QWidget *group = nullptr;
        if (entry.groupBody != nullptr) {
            group = entry.groupBody->parentWidget();
            if (group != nullptr) {
                allGroups.insert(group);
            }
        }

        const bool match = needle.isEmpty() || entry.searchText.contains(needle);
        entry.row->setVisible(match);
        if (match && group != nullptr) {
            groupsWithMatches.insert(group);
        }
    }

    for (QWidget *group : allGroups) {
        group->setVisible(needle.isEmpty() || groupsWithMatches.contains(group));
    }
}
