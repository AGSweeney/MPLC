/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("MPLC Studio");
    QApplication::setApplicationDisplayName("MPLC Studio");
    QApplication::setOrganizationName("MPLC");
    QApplication::setOrganizationDomain("mplc.local");

    if (QStyle *fusion = QStyleFactory::create("Fusion")) {
        app.setStyle(fusion);
    }
    QFont appFont(QStringLiteral("Segoe UI"), -1, QFont::Normal);
    appFont.setPixelSize(12);
    appFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(appFont);
    app.setWindowIcon(QIcon(":/icons/app-icon-256.png"));

    MainWindow window;
    window.setWindowIcon(QIcon(":/icons/app-icon-256.png"));
    window.show();

    return app.exec();
}
