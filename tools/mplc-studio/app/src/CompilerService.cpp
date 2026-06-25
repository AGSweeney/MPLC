/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CompilerService.h"

#include "mplc_compile.h"

#include <QDir>
#include <QFileInfo>
#include <QtConcurrent>
#include <QFutureWatcher>

namespace {

QString formatDiagnostics(const diag_ctx_t &diag)
{
    QString text;
    for (uint32_t i = 0; i < diag.count; i++) {
        text += QString("%1:%2: %3\n")
                    .arg(QString::fromLocal8Bit(diag.msgs[i].file))
                    .arg(diag.msgs[i].line)
                    .arg(QString::fromLocal8Bit(diag.msgs[i].message));
    }
    return text.trimmed();
}

} // namespace

CompilerService::CompilerService(QObject *parent)
    : QObject(parent)
{
}

CompilerService::~CompilerService()
{
    if (m_watcher != nullptr) {
        m_watcher->waitForFinished();
    }
}

CompileResult CompilerService::runCompile(const QString &sourcePath, const QString &outputPath)
{
    CompileResult result;
    result.sourcePath = sourcePath;
    result.outputPath = outputPath;

    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        result.combinedOutput = QString("Source file not found: %1").arg(sourcePath);
        return result;
    }

    const QString nativeSource = QDir::toNativeSeparators(sourceInfo.absoluteFilePath());
    const QString nativeOutput = QDir::toNativeSeparators(QFileInfo(outputPath).absoluteFilePath());

    diag_ctx_t diag;
    diag_init(&diag);

    const QByteArray inputUtf8 = QFile::encodeName(nativeSource);
    const QByteArray outputUtf8 = QFile::encodeName(nativeOutput);
    const int rc = mplc_compile_file(inputUtf8.constData(), outputUtf8.constData(), &diag);

    if (rc != 0) {
        result.combinedOutput = formatDiagnostics(diag);
        if (result.combinedOutput.isEmpty()) {
            result.combinedOutput = QString("Compile failed (code %1)").arg(rc);
        }
        diag_free(&diag);
        return result;
    }

    diag_free(&diag);

    QFileInfo outputInfo(nativeOutput);
    if (!outputInfo.exists() || outputInfo.size() == 0) {
        result.combinedOutput = "Compiler reported success but output file was not created.";
        return result;
    }

    result.success = true;
    result.combinedOutput = QString("Compiled %1 -> %2 (%3 bytes)")
                                .arg(nativeSource, nativeOutput)
                                .arg(outputInfo.size());
    return result;
}

bool CompilerService::compile(const QString &sourcePath, const QString &outputPath, QString &combinedOutput)
{
    combinedOutput.clear();
    const CompileResult result = runCompile(sourcePath, outputPath);
    combinedOutput = result.combinedOutput;
    return result.success;
}

void CompilerService::compileAsync(const QString &sourcePath, const QString &outputPath)
{
    if (m_busy) {
        return;
    }

    m_busy = true;
    emit compileStarted();

    m_watcher = new QFutureWatcher<CompileResult>(this);
    connect(m_watcher, &QFutureWatcher<CompileResult>::finished, this, [this]() {
        const CompileResult result = m_watcher->result();
        m_busy = false;
        m_watcher->deleteLater();
        m_watcher = nullptr;
        emit compileFinished(result);
    });

    m_watcher->setFuture(QtConcurrent::run([sourcePath, outputPath]() {
        return CompilerService::runCompile(sourcePath, outputPath);
    }));
}

void CompilerService::waitForIdle()
{
    if (m_watcher != nullptr) {
        m_watcher->waitForFinished();
    }
}
