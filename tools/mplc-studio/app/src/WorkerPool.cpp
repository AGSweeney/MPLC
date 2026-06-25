/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "WorkerPool.h"

#include <QThread>
#include <QThreadPool>

namespace WorkerPool {

void configure()
{
    QThreadPool *pool = QThreadPool::globalInstance();
    const int ideal = QThread::idealThreadCount();
    pool->setMaxThreadCount(qMax(4, ideal));
}

} // namespace WorkerPool
