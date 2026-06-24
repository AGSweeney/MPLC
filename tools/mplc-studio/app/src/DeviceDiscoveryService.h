/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "DiscoveredDevice.h"

#include <QObject>
#include <QSet>
#include <QVector>

class QUdpSocket;
class QNetworkAccessManager;

class DeviceDiscoveryService : public QObject
{
    Q_OBJECT

public:
    explicit DeviceDiscoveryService(QObject *parent = nullptr);

    void setCredentials(const QString &user, const QString &password);
    void startDiscovery(int timeoutMs = 3500);
    void cancelDiscovery();

signals:
    void logMessage(const QString &message);
    void discoveryFinished(const QVector<DiscoveredDevice> &devices);

private:
    void finishDiscovery();
    void sendDiscoveryPackets();
    void handleDatagram();
    void startMplcProbes();
    void probeMplcDevice(const QString &ip);

    QUdpSocket *m_udp = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    QString m_user;
    QString m_password;
    QVector<DiscoveredDevice> m_devices;
    QSet<QString> m_seenIps;
    int m_pendingProbes = 0;
    bool m_active = false;
};
