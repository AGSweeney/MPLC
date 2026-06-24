/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QObject>
#include <QString>

class CompilerService : public QObject
{
    Q_OBJECT

public:
    explicit CompilerService(QObject *parent = nullptr);

    bool compile(const QString &sourcePath, const QString &outputPath, QString &combinedOutput);
};
