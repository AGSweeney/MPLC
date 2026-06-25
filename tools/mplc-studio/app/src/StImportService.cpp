/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "StImportService.h"

#include <QtConcurrent>
#include <QFutureWatcher>

StImportService::StImportService(QObject *parent)
    : QObject(parent)
{
}

StImportService::~StImportService()
{
    if (m_watcher != nullptr) {
        m_watcher->waitForFinished();
    }
}

void StImportService::importAsync(const QString &source)
{
    if (m_busy) {
        return;
    }

    m_busy = true;
    emit importStarted();

    m_watcher = new QFutureWatcher<StLadderImporter::Result>(this);
    connect(m_watcher, &QFutureWatcher<StLadderImporter::Result>::finished, this, [this]() {
        const StLadderImporter::Result result = m_watcher->result();
        m_busy = false;
        m_watcher->deleteLater();
        m_watcher = nullptr;
        emit importFinished(result);
    });

    m_watcher->setFuture(QtConcurrent::run([source]() {
        return StLadderImporter::importSource(source);
    }));
}

void StImportService::waitForIdle()
{
    if (m_watcher != nullptr) {
        m_watcher->waitForFinished();
    }
}
