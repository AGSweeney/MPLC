/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QWidget>

class LadderScene;

class LadderToolbox : public QWidget
{
    Q_OBJECT

public:
    explicit LadderToolbox(QWidget *parent = nullptr);

    void setScene(LadderScene *scene);
    void setExpanded(bool expanded);
    bool isExpanded() const;

signals:
    void expandedChanged(bool expanded);

private:
    LadderScene *m_scene = nullptr;
    QWidget *m_expandedPanel = nullptr;
    QWidget *m_collapsedTab = nullptr;
    bool m_expanded = true;
};
