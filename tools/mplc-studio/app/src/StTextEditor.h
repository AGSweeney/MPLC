/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "DocumentEditor.h"

class QPlainTextEdit;

class StTextEditor : public DocumentEditor
{
    Q_OBJECT

public:
    explicit StTextEditor(QWidget *parent = nullptr);

    QPlainTextEdit *editor() const { return m_editor; }

    bool loadFromPath(const QString &path) override;
    bool saveToPath(const QString &path) override;
    bool isDirty() const override { return m_dirty; }
    void markClean() override;
    void newDocument() override;
    QString suggestedSaveFilter() const override;
    QString suggestedOpenFilter() const override;
    QString compileInputPath(QString *tempCleanupPath) override;

private:
    QPlainTextEdit *m_editor = nullptr;
    QString m_filePath;
};
