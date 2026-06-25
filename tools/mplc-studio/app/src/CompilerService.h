/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QString>

struct CompileResult {
    bool success = false;
    QString combinedOutput;
    QString sourcePath;
    QString outputPath;
};

class CompilerService : public QObject
{
    Q_OBJECT

public:
    explicit CompilerService(QObject *parent = nullptr);
    ~CompilerService() override;

    bool isBusy() const { return m_busy; }

    bool compile(const QString &sourcePath, const QString &outputPath, QString &combinedOutput);
    void compileAsync(const QString &sourcePath, const QString &outputPath);
    void waitForIdle();

signals:
    void compileStarted();
    void compileFinished(const CompileResult &result);

private:
    static CompileResult runCompile(const QString &sourcePath, const QString &outputPath);

    bool m_busy = false;
    QFutureWatcher<CompileResult> *m_watcher = nullptr;
};
