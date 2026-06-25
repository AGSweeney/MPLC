/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LadderInstructionBar.h"
#include "LadderDraggableButton.h"
#include "LadderMime.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace {

QWidget *makeSeparator(QWidget *parent)
{
    auto *line = new QFrame(parent);
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Plain);
    line->setStyleSheet(QStringLiteral("color: #b0b0b0;"));
    line->setFixedWidth(1);
    return line;
}

} // namespace

LadderInstructionBar::LadderInstructionBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("LadderInstructionBar");
    setFixedHeight(42);
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(4);

    auto *title = new QLabel(QStringLiteral("Ladder Instructions"), this);
    title->setObjectName("LadderBarTitle");
    layout->addWidget(title);
    layout->addWidget(makeSeparator(this));

    auto addBtn = [this, layout](const QString &label, const QString &tip, const QString &payload, auto slot) {
        auto *button = new LadderDraggableButton(label, payload, this);
        button->setObjectName("LadderBarButton");
        button->setToolTip(tip + QStringLiteral(" — click to add on selected rung, drag onto canvas"));
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumWidth(52);
        button->setMinimumHeight(30);
        connect(button, &QPushButton::clicked, this, slot);
        layout->addWidget(button);
        return button;
    };

    addBtn(QStringLiteral("| |"),
           QStringLiteral("Add normally-open contact"),
           LadderMime::contactPayload(false),
           [this]() { emit addContact(false); });
    addBtn(QStringLiteral("|/|"),
           QStringLiteral("Add normally-closed contact"),
           LadderMime::contactPayload(true),
           [this]() { emit addContact(true); });
    addBtn(QStringLiteral("( )"),
           QStringLiteral("Add output coil"),
           LadderMime::coilPayload(0),
           [this]() { emit addCoil(); });
    addBtn(QStringLiteral("(S)"),
           QStringLiteral("Add set coil"),
           LadderMime::coilPayload(1),
           [this]() { emit addSetCoil(); });
    addBtn(QStringLiteral("(R)"),
           QStringLiteral("Add reset coil"),
           LadderMime::coilPayload(2),
           [this]() { emit addResetCoil(); });

    layout->addWidget(makeSeparator(this));
    addBtn(QStringLiteral("+ Rung"), QStringLiteral("Add new rung"), QString(), [this]() { emit addRung(); });
    addBtn(QStringLiteral("Branch"),
           QStringLiteral("Input: select row-0 contacts, then Branch. Output: select coil(s). Nested: select branch-row contacts, then Branch."),
           QString(),
           [this]() { emit surroundWithBranch(); });
    addBtn(QStringLiteral("+ Leg"),
           QStringLiteral("Add a parallel branch row with a contact (requires an input branch — click Branch first)"),
           QString(),
           [this]() { emit addParallelBranchLeg(); });
    addBtn(QStringLiteral("− Branch"),
           QStringLiteral("Remove selected input branch (OR tab) or output branch (OUT tab)"),
           QString(),
           [this]() { emit removeBranch(); });
    addBtn(QStringLiteral("Del Elem"),
           QStringLiteral("Delete selected elements"),
           QString(),
           [this]() { emit deleteSelection(); });
    addBtn(QStringLiteral("Del Rung"),
           QStringLiteral("Delete selected rung (Del when nothing selected)"),
           QString(),
           [this]() { emit deleteRung(); });

    layout->addStretch(1);

    layout->addWidget(makeSeparator(this));

    auto *stButton = new QPushButton(QStringLiteral("View ST"), this);
    stButton->setObjectName("LadderBarButton");
    stButton->setToolTip(QStringLiteral("View generated Structured Text"));
    connect(stButton, &QPushButton::clicked, this, &LadderInstructionBar::viewGeneratedSt);
    layout->addWidget(stButton);

    auto *varsButton = new QPushButton(QStringLiteral("Variables"), this);
    varsButton->setObjectName("LadderBarButton");
    varsButton->setToolTip(QStringLiteral("Browse and edit global variables"));
    connect(varsButton, &QPushButton::clicked, this, &LadderInstructionBar::browseVariables);
    layout->addWidget(varsButton);
}

QWidget *LadderInstructionBar::makeButton(const QString &label, const QString &tooltip)
{
    auto *button = new QPushButton(label, this);
    button->setObjectName("LadderBarButton");
    button->setToolTip(tooltip);
    return button;
}
