/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "ladder/StLadderImporter.h"

#include <QFutureWatcher>
#include <QObject>

class StImportService : public QObject
{
    Q_OBJECT

public:
    explicit StImportService(QObject *parent = nullptr);
    ~StImportService() override;

    bool isBusy() const { return m_busy; }
    void importAsync(const QString &source);
    void waitForIdle();

signals:
    void importStarted();
    void importFinished(const StLadderImporter::Result &result);

private:
    bool m_busy = false;
    QFutureWatcher<StLadderImporter::Result> *m_watcher = nullptr;
};
