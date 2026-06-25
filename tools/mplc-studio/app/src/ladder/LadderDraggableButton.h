/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "LadderMime.h"

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>

class LadderDraggableButton : public QPushButton
{
public:
    explicit LadderDraggableButton(const QString &label, const QString &payload, QWidget *parent = nullptr)
        : QPushButton(label, parent)
        , m_payload(payload)
    {
        setCursor(Qt::PointingHandCursor);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        m_pressPos = event->pos();
        QPushButton::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (m_payload.isEmpty()) {
            return;
        }
        if (!(event->buttons() & Qt::LeftButton)) {
            return;
        }
        if ((event->pos() - m_pressPos).manhattanLength() < QApplication::startDragDistance()) {
            return;
        }

        auto *drag = new QDrag(this);
        auto *mime = new QMimeData();
        mime->setData(LadderMime::mimeType(), m_payload.toUtf8());
        drag->setMimeData(mime);
        drag->exec(Qt::CopyAction);
    }

private:
    QPoint m_pressPos;
    QString m_payload;
};
