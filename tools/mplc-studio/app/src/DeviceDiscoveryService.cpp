/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DeviceDiscoveryService.h"

#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkDatagram>
#include <QSet>
#include <QTimer>
#include <QUdpSocket>
#include <QtEndian>
#include <algorithm>

namespace {

static constexpr quint32 kVerifyFromNdkToPc = 0x4E455442u; // NETB
static constexpr quint16 kNetBurnerDiscoveryPort = 20034;

QString formatMac(const quint8 *bytes, int length)
{
    if (length < 6) {
        return QString();
    }

    return QString("%1:%2:%3:%4:%5:%6")
        .arg(bytes[0], 2, 16, QLatin1Char('0'))
        .arg(bytes[1], 2, 16, QLatin1Char('0'))
        .arg(bytes[2], 2, 16, QLatin1Char('0'))
        .arg(bytes[3], 2, 16, QLatin1Char('0'))
        .arg(bytes[4], 2, 16, QLatin1Char('0'))
        .arg(bytes[5], 2, 16, QLatin1Char('0'))
        .toUpper();
}

bool looksLikeMplcUploadPage(const QString &body)
{
    return body.contains("MPLC Program Upload", Qt::CaseInsensitive)
        || body.contains("mplc_upload.html", Qt::CaseInsensitive)
        || body.contains("mplc_update.html", Qt::CaseInsensitive);
}

} // namespace

DeviceDiscoveryService::DeviceDiscoveryService(QObject *parent)
    : QObject(parent)
    , m_udp(new QUdpSocket(this))
    , m_network(new QNetworkAccessManager(this))
{
    connect(m_udp, &QUdpSocket::readyRead, this, &DeviceDiscoveryService::handleDatagram);
}

void DeviceDiscoveryService::setCredentials(const QString &user, const QString &password)
{
    m_user = user;
    m_password = password;
}

void DeviceDiscoveryService::startDiscovery(int timeoutMs)
{
    cancelDiscovery();

    m_active = true;
    m_devices.clear();
    m_seenIps.clear();
    m_pendingProbes = 0;

    if (!m_udp->bind(QHostAddress::AnyIPv4, kNetBurnerDiscoveryPort + 1, QUdpSocket::ShareAddress)) {
        if (!m_udp->bind(QHostAddress::AnyIPv4, 0, QUdpSocket::ShareAddress)) {
            emit logMessage(QString("UDP bind failed: %1").arg(m_udp->errorString()));
            finishDiscovery();
            return;
        }
    }

    emit logMessage("Broadcasting NetBurner discovery...");
    sendDiscoveryPackets();

    QTimer::singleShot(timeoutMs, this, [this]() {
        if (!m_active) {
            return;
        }
        emit logMessage(QString("Discovery listen complete (%1 NetBurner response(s)).")
                            .arg(m_seenIps.size()));
        startMplcProbes();
    });
}

void DeviceDiscoveryService::cancelDiscovery()
{
    m_active = false;
    m_pendingProbes = 0;
    if (m_udp->state() == QAbstractSocket::BoundState) {
        m_udp->close();
    }
}

void DeviceDiscoveryService::sendDiscoveryPackets()
{
    const QByteArray request("BURNR");

    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if (!iface.flags().testFlag(QNetworkInterface::IsUp)
            || iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            continue;
        }

        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol) {
                continue;
            }

            const QHostAddress broadcast = entry.broadcast();
            if (broadcast.isNull()) {
                continue;
            }

            m_udp->writeDatagram(request, broadcast, kNetBurnerDiscoveryPort);
        }
    }

    m_udp->writeDatagram(request, QHostAddress::Broadcast, kNetBurnerDiscoveryPort);
}

void DeviceDiscoveryService::handleDatagram()
{
    while (m_udp->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_udp->receiveDatagram(2048);
        const QByteArray data = datagram.data();
        if (data.size() < 5) {
            continue;
        }

        const quint32 key = qFromBigEndian<quint32>(data.constData());
        if (key != kVerifyFromNdkToPc) {
            continue;
        }

        const QString ip = datagram.senderAddress().toString();
        if (ip.isEmpty() || m_seenIps.contains(ip)) {
            continue;
        }

        DiscoveredDevice device;
        device.ip = ip;
        device.netburnerDevice = true;
        device.details = "NetBurner device";

        if (data.size() >= 114) {
            device.mac = formatMac(reinterpret_cast<const quint8 *>(data.constData() + 108), 6);
        }

        if (data.contains("NEWCONFIG")) {
            device.details = "NetBurner (NNDK config)";
        }

        m_seenIps.insert(ip);
        m_devices.push_back(device);
        emit logMessage(QString("Found NetBurner at %1%2")
                            .arg(ip, device.mac.isEmpty() ? QString() : QString(" MAC %1").arg(device.mac)));
    }
}

void DeviceDiscoveryService::startMplcProbes()
{
    if (!m_active) {
        return;
    }

    if (m_devices.isEmpty()) {
        finishDiscovery();
        return;
    }

    emit logMessage("Probing devices for MPLC upload support...");
    m_pendingProbes = m_devices.size();

    for (DiscoveredDevice &device : m_devices) {
        probeMplcDevice(device.ip);
    }
}

void DeviceDiscoveryService::probeMplcDevice(const QString &ip)
{
    QNetworkRequest request(QUrl(QString("http://%1/mplc_update.html").arg(ip)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "MPLCStudio/0.1");
    request.setTransferTimeout(4000);

    if (!m_user.isEmpty()) {
        const QByteArray token = QString("%1:%2").arg(m_user, m_password).toUtf8().toBase64();
        request.setRawHeader("Authorization", "Basic " + token);
    }

    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, ip]() {
        const QByteArray body = reply->readAll();
        const QString text = QString::fromUtf8(body);
        const bool httpOk = reply->error() == QNetworkReply::NoError;
        const bool mplcPage = looksLikeMplcUploadPage(text);

        for (DiscoveredDevice &device : m_devices) {
            if (device.ip == ip) {
                if (httpOk && mplcPage) {
                    device.mplcSupported = true;
                    device.details = "MPLC upload endpoint";
                    emit logMessage(QString("%1 supports MPLC upload.").arg(ip));
                } else {
                    device.details = "NetBurner (no MPLC upload page)";
                    emit logMessage(QString("%1 is NetBurner but not an MPLC target.").arg(ip));
                }
                break;
            }
        }

        reply->deleteLater();

        if (!m_active) {
            return;
        }

        m_pendingProbes = qMax(0, m_pendingProbes - 1);
        if (m_pendingProbes == 0) {
            finishDiscovery();
        }
    });
}

void DeviceDiscoveryService::finishDiscovery()
{
    if (!m_active) {
        return;
    }

    m_active = false;
    if (m_udp->state() == QAbstractSocket::BoundState) {
        m_udp->close();
    }

    std::sort(m_devices.begin(), m_devices.end(), [](const DiscoveredDevice &a, const DiscoveredDevice &b) {
        if (a.mplcSupported != b.mplcSupported) {
            return a.mplcSupported > b.mplcSupported;
        }
        return a.ip < b.ip;
    });

    emit discoveryFinished(m_devices);
}
