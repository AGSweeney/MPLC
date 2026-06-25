/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QIcon>
#include <QToolButton>
#include <QString>

class LadderToolboxItem : public QToolButton
{
public:
    explicit LadderToolboxItem(const QIcon &icon, const QString &label, const QString &payload, QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QPoint m_pressPos;
    QString m_payload;
};
