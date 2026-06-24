/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CompilerService.h"

#include "mplc_compile.h"

#include <QDir>
#include <QFileInfo>

CompilerService::CompilerService(QObject *parent)
    : QObject(parent)
{
}

static QString formatDiagnostics(const diag_ctx_t &diag)
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

bool CompilerService::compile(const QString &sourcePath, const QString &outputPath, QString &combinedOutput)
{
    combinedOutput.clear();

    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        combinedOutput = QString("Source file not found: %1").arg(sourcePath);
        return false;
    }

    const QString nativeSource = QDir::toNativeSeparators(sourceInfo.absoluteFilePath());
    const QString nativeOutput = QDir::toNativeSeparators(QFileInfo(outputPath).absoluteFilePath());

    diag_ctx_t diag;
    diag_init(&diag);

    const QByteArray inputUtf8 = QFile::encodeName(nativeSource);
    const QByteArray outputUtf8 = QFile::encodeName(nativeOutput);
    const int rc = mplc_compile_file(inputUtf8.constData(), outputUtf8.constData(), &diag);

    if (rc != 0) {
        combinedOutput = formatDiagnostics(diag);
        if (combinedOutput.isEmpty()) {
            combinedOutput = QString("Compile failed (code %1)").arg(rc);
        }
        diag_free(&diag);
        return false;
    }

    diag_free(&diag);

    QFileInfo outputInfo(nativeOutput);
    if (!outputInfo.exists() || outputInfo.size() == 0) {
        combinedOutput = "Compiler reported success but output file was not created.";
        return false;
    }

    combinedOutput = QString("Compiled %1 -> %2 (%3 bytes)")
                         .arg(nativeSource, nativeOutput)
                         .arg(outputInfo.size());
    return true;
}
