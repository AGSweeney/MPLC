/*

 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>

 * SPDX-License-Identifier: BSD-3-Clause

 */



#include "LadderToolbox.h"

#include "LadderMime.h"

#include "LadderModel.h"

#include "LadderScene.h"

#include "LadderSymbolIcons.h"

#include "LadderToolboxItem.h"

#include "StudioTheme.h"



#include <QFrame>

#include <QHBoxLayout>

#include <QLabel>

#include <QMouseEvent>

#include <QPainter>

#include <QScrollArea>

#include <QToolButton>

#include <QVBoxLayout>



namespace {



constexpr int kCollapsedTabWidth = 24;



class CollapsedTab final : public QWidget

{

public:

    explicit CollapsedTab(QWidget *parent = nullptr)

        : QWidget(parent)
    {
        setObjectName(QStringLiteral("StToolboxCollapsedTab"));
        setFixedWidth(kCollapsedTabWidth);

        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

        setCursor(Qt::PointingHandCursor);

        setToolTip("Show Ladder Toolbox");

    }



    std::function<void()> onActivate;



protected:

    void paintEvent(QPaintEvent *event) override

    {

        QWidget::paintEvent(event);

        QPainter painter(this);

        painter.setPen(QColor("#2d325a"));

        QFont font = painter.font();

        font.setBold(true);

        font.setPointSize(9);

        painter.setFont(font);

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



QWidget *makeGroup(QWidget *parent, const QString &title, QVBoxLayout *layout, const QList<QWidget *> &items)

{

    auto *group = new QWidget(parent);

    group->setObjectName(QStringLiteral("LadderToolboxGroup"));

    auto *groupLayout = new QVBoxLayout(group);

    groupLayout->setContentsMargins(0, 0, 0, 6);

    groupLayout->setSpacing(2);



    auto *header = new QLabel(title, group);

    header->setObjectName(QStringLiteral("LadderToolboxGroupTitle"));

    groupLayout->addWidget(header);



    for (QWidget *item : items) {

        groupLayout->addWidget(item);

    }



    layout->addWidget(group);

    return group;

}



} // namespace



LadderToolbox::LadderToolbox(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(kCollapsedTabWidth);
    setMaximumWidth(300);

    auto *rootLayout = new QHBoxLayout(this);

    rootLayout->setContentsMargins(0, 0, 0, 0);

    rootLayout->setSpacing(0);



    m_expandedPanel = new QWidget(this);

    m_expandedPanel->setObjectName("StInstructionToolbox");

    auto *panelLayout = new QVBoxLayout(m_expandedPanel);

    panelLayout->setContentsMargins(8, 8, 8, 8);

    panelLayout->setSpacing(6);



    auto *headerRow = new QHBoxLayout();

    auto *title = new QLabel(QStringLiteral("Ladder Toolbox"), m_expandedPanel);

    title->setObjectName("RibbonGroupCaption");

    auto *collapseButton = new QToolButton(m_expandedPanel);

    collapseButton->setIcon(StudioTheme::renderSvgIcon(QStringLiteral(":/icons/chevron-right.svg"), 12, QColor("#2d325a")));

    collapseButton->setAutoRaise(true);

    collapseButton->setToolTip(QStringLiteral("Hide toolbox"));

    connect(collapseButton, &QToolButton::clicked, this, [this]() { setExpanded(false); });

    headerRow->addWidget(title);

    headerRow->addStretch(1);

    headerRow->addWidget(collapseButton);

    panelLayout->addLayout(headerRow);



    auto *scroll = new QScrollArea(m_expandedPanel);

    scroll->setObjectName(QStringLiteral("LadderToolboxScroll"));

    scroll->setWidgetResizable(true);

    scroll->setFrameShape(QFrame::NoFrame);

    auto *scrollBody = new QWidget(scroll);

    auto *scrollLayout = new QVBoxLayout(scrollBody);

    scrollLayout->setContentsMargins(0, 0, 0, 0);

    scrollLayout->setSpacing(6);



    const auto makeItem = [scrollBody, this](const QIcon &icon, const QString &label, const QString &payload, auto action) {

        auto *item = new LadderToolboxItem(icon, label, payload, scrollBody);

        item->setToolTip(QStringLiteral("%1 — click or drag onto canvas").arg(label));

        connect(item, &QToolButton::clicked, this, action);

        return item;

    };



    makeGroup(scrollBody,

              QStringLiteral("Contacts"),

              scrollLayout,

              {makeItem(LadderSymbolIcons::icon(LadderSymbolKind::ContactNo),

                        QStringLiteral("Normally Open"),

                        LadderMime::contactPayload(false),

                        [this]() {

                            if (m_scene) {

                                m_scene->addContactToSelectedRung(false);

                            }

                        }),

               makeItem(LadderSymbolIcons::icon(LadderSymbolKind::ContactNc),

                        QStringLiteral("Normally Closed"),

                        LadderMime::contactPayload(true),

                        [this]() {

                            if (m_scene) {

                                m_scene->addContactToSelectedRung(true);

                            }

                        })});



    makeGroup(scrollBody,

              QStringLiteral("Coils"),

              scrollLayout,

              {makeItem(LadderSymbolIcons::icon(LadderSymbolKind::CoilNormal),

                        QStringLiteral("Output"),

                        LadderMime::coilPayload(0),

                        [this]() {

                            if (m_scene) {

                                m_scene->addCoilToSelectedRung(LadderCoilKind::Normal);

                            }

                        }),

               makeItem(LadderSymbolIcons::icon(LadderSymbolKind::CoilSet),

                        QStringLiteral("Set (S)"),

                        LadderMime::coilPayload(1),

                        [this]() {

                            if (m_scene) {

                                m_scene->addCoilToSelectedRung(LadderCoilKind::Set);

                            }

                        }),

               makeItem(LadderSymbolIcons::icon(LadderSymbolKind::CoilReset),

                        QStringLiteral("Reset (R)"),

                        LadderMime::coilPayload(2),

                        [this]() {

                            if (m_scene) {

                                m_scene->addCoilToSelectedRung(LadderCoilKind::Reset);

                            }

                        }),

               makeItem(LadderSymbolIcons::icon(LadderSymbolKind::CoilNormal),

                        QStringLiteral("Branch Output"),

                        QString(),

                        [this]() {

                            if (m_scene) {

                                m_scene->addOutputBranchCoilToSelectedRung(LadderCoilKind::Normal);

                            }

                        })});



    makeGroup(scrollBody,

              QStringLiteral("Rungs"),

              scrollLayout,

              {makeItem(LadderSymbolIcons::icon(LadderSymbolKind::AddRung),

                        QStringLiteral("Add Rung"),

                        QString(),

                        [this]() {

                            if (m_scene) {

                                m_scene->addRung();

                            }

                        }),

               makeItem(LadderSymbolIcons::icon(LadderSymbolKind::SurroundBranch),

                        QStringLiteral("Surround with Branch"),

                        QString(),

                        [this]() {

                            if (m_scene) {

                                m_scene->surroundSelectionWithBranch();

                            }

                        }),

               makeItem(LadderSymbolIcons::icon(LadderSymbolKind::SurroundBranch),

                        QStringLiteral("+ Branch Leg"),

                        QString(),

                        [this]() {

                            if (m_scene) {

                                m_scene->addParallelBranchLeg(false);

                            }

                        }),

               makeItem(LadderSymbolIcons::icon(LadderSymbolKind::RemoveBranch),

                        QStringLiteral("Remove Branch"),

                        QString(),

                        [this]() {

                            if (m_scene) {

                                m_scene->removeBranchFromSelectedRung();

                            }

                        })});



    QList<QWidget *> timerItems;

    QList<QWidget *> counterItems;

    QList<QWidget *> triggerItems;

    for (const LadderFbDescriptor &desc : LadderModel::functionBlockCatalog()) {

        auto *item = makeItem(LadderSymbolIcons::icon(LadderSymbolKind::FunctionBlock, desc.typeName),

                              desc.typeName,

                              LadderMime::functionBlockPayload(desc.typeName),

                              [this, typeName = desc.typeName]() {

                                  if (m_scene) {

                                      m_scene->addFunctionBlockToSelectedRung(typeName);

                                  }

                              });

        if (desc.typeName.startsWith('T')) {

            timerItems.push_back(item);

        } else if (desc.typeName.startsWith('C')) {

            counterItems.push_back(item);

        } else {

            triggerItems.push_back(item);

        }

    }

    makeGroup(scrollBody, QStringLiteral("Timers"), scrollLayout, timerItems);

    makeGroup(scrollBody, QStringLiteral("Counters"), scrollLayout, counterItems);

    makeGroup(scrollBody, QStringLiteral("Triggers / Latches"), scrollLayout, triggerItems);



    scrollLayout->addStretch(1);

    scroll->setWidget(scrollBody);

    panelLayout->addWidget(scroll, 1);



    m_collapsedTab = new CollapsedTab(this);

    auto *collapsed = static_cast<CollapsedTab *>(m_collapsedTab);

    collapsed->onActivate = [this]() { setExpanded(true); };



    rootLayout->addWidget(m_expandedPanel, 1);

    rootLayout->addWidget(m_collapsedTab);

    setExpanded(true);

}



void LadderToolbox::setScene(LadderScene *scene)

{

    m_scene = scene;

}



void LadderToolbox::setExpanded(bool expanded)

{

    if (m_expanded == expanded) {

        return;

    }

    m_expanded = expanded;

    m_expandedPanel->setVisible(expanded);
    m_collapsedTab->setVisible(!expanded);

    if (expanded) {
        setMinimumWidth(220);
        setMaximumWidth(300);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    } else {
        setMinimumWidth(kCollapsedTabWidth);
        setMaximumWidth(kCollapsedTabWidth);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }

    emit expandedChanged(expanded);
}



bool LadderToolbox::isExpanded() const

{

    return m_expanded;

}


