/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QWidget>

class DocumentEditor : public QWidget
{
    Q_OBJECT

public:
    enum class Kind {
        StructuredText,
        Ladder
    };

    explicit DocumentEditor(Kind kind, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_kind(kind)
    {
    }

    Kind editorKind() const { return m_kind; }

    virtual bool loadFromPath(const QString &path) = 0;
    virtual bool saveToPath(const QString &path) = 0;
    virtual bool isDirty() const = 0;
    virtual void markClean() = 0;
    virtual void newDocument() = 0;
    virtual QString suggestedSaveFilter() const = 0;
    virtual QString suggestedOpenFilter() const = 0;
    virtual QString compileInputPath(QString *tempCleanupPath) = 0;

signals:
    void dirtyChanged(bool dirty);
    void statusMessage(const QString &message);

protected:
    void setDirty(bool dirty)
    {
        if (m_dirty != dirty) {
            m_dirty = dirty;
            emit dirtyChanged(dirty);
        }
    }

    Kind m_kind;
    bool m_dirty = false;
};
