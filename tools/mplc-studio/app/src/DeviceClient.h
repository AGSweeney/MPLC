/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;

class DeviceClient : public QObject
{
    Q_OBJECT

public:
    explicit DeviceClient(QObject *parent = nullptr);

    void setDeviceHost(const QString &host);
    QString deviceHost() const;

    void setCredentials(const QString &user, const QString &password);

    void uploadPackage(const QString &packagePath);
    void rebootDevice();

signals:
    void logMessage(const QString &message);
    void uploadFinished(bool success, const QString &detail);
    void rebootFinished(bool success, const QString &detail);
    void operationFailed(const QString &detail);

private:
    void sendRebootRequest();
    QUrl baseUrl() const;

    QNetworkAccessManager *m_network;
    QString m_deviceHost;
    QString m_user;
    QString m_password;
};
