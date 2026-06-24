/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DeviceClient.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

namespace {

QString extractHtmlError(const QString &html)
{
    const QRegularExpression re("Upload failed \\(error (-?\\d+)\\)", QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = re.match(html);
    if (match.hasMatch()) {
        const int code = match.captured(1).toInt();
        switch (code) {
        case -1:
            return "Upload failed: device did not receive a file in the POST body. "
                   "Check that the request uses multipart form field \"plcfile\".";
        case -10:
            return "Upload failed: flash filesystem not mounted (error -10).";
        case -11:
            return "Upload failed: could not open package file on device flash (error -11).";
        case -12:
            return "Upload failed: flash write error (error -12).";
        case -13:
            return "Upload failed: package exceeds max size (error -13).";
        case -14:
            return "Upload failed: read error while receiving upload (error -14).";
        case -15:
            return "Upload failed: empty upload (error -15).";
        case -16:
            return "Upload failed: no file field received in multipart POST (error -16).";
        default:
            return QString("Upload failed (device error %1).").arg(code);
        }
    }

    if (html.contains("Upload Complete", Qt::CaseInsensitive)) {
        return QString();
    }

    return QString();
}

QByteArray buildMultipartBody(const QByteArray &fileData, const QString &filename, QString &boundary)
{
    boundary = QString("----MPLCStudio%1").arg(QDateTime::currentMSecsSinceEpoch());
    const QByteArray boundaryBytes = boundary.toUtf8();
    const QByteArray fileNameBytes = filename.toUtf8();

    QByteArray body;
    body += "--";
    body += boundaryBytes;
    body += "\r\n";
    body += "Content-Disposition: form-data; name=\"plcfile\"; filename=\"";
    body += fileNameBytes;
    body += "\"\r\n";
    body += "Content-Type: application/octet-stream\r\n";
    body += "\r\n";
    body += fileData;
    body += "\r\n";
    body += "--";
    body += boundaryBytes;
    body += "--\r\n";
    return body;
}

} // namespace

DeviceClient::DeviceClient(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

void DeviceClient::setDeviceHost(const QString &host)
{
    m_deviceHost = host.trimmed();
}

QString DeviceClient::deviceHost() const
{
    return m_deviceHost;
}

void DeviceClient::setCredentials(const QString &user, const QString &password)
{
    m_user = user;
    m_password = password;
}

QUrl DeviceClient::baseUrl() const
{
    QString host = m_deviceHost;
    if (host.startsWith("http://", Qt::CaseInsensitive) || host.startsWith("https://", Qt::CaseInsensitive)) {
        return QUrl(host);
    }
    return QUrl(QString("http://%1").arg(host));
}

void DeviceClient::uploadPackage(const QString &packagePath)
{
    if (m_deviceHost.isEmpty()) {
        emit operationFailed("Target device address is not set.");
        return;
    }

    QFile file(packagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit operationFailed(QString("Cannot open package file: %1").arg(packagePath));
        return;
    }

    const QByteArray fileData = file.readAll();
    file.close();

    if (fileData.isEmpty()) {
        emit operationFailed(QString("Package file is empty: %1").arg(packagePath));
        return;
    }

    QString boundary;
    const QByteArray body = buildMultipartBody(fileData, QFileInfo(packagePath).fileName(), boundary);

    QNetworkRequest request(baseUrl().resolved(QUrl("/mplc_upload.html")));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                        QString("multipart/form-data; boundary=%1").arg(boundary));
    request.setHeader(QNetworkRequest::UserAgentHeader, "MPLCStudio/0.1");

    if (!m_user.isEmpty()) {
        const QByteArray token = QString("%1:%2").arg(m_user, m_password).toUtf8().toBase64();
        request.setRawHeader("Authorization", "Basic " + token);
    }

    emit logMessage(QString("Uploading %1 (%2 bytes) to %3 ...")
                        .arg(packagePath)
                        .arg(fileData.size())
                        .arg(request.url().toString()));

    QNetworkReply *reply = m_network->post(request, body);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray bodyBytes = reply->readAll();
        const bool httpOk = reply->error() == QNetworkReply::NoError;
        const QString text = QString::fromUtf8(bodyBytes);
        const QString parsedError = extractHtmlError(text);
        const bool contentOk = parsedError.isEmpty() && text.contains("Upload Complete", Qt::CaseInsensitive);

        if (httpOk && contentOk) {
            emit logMessage("Upload completed successfully.");
            emit uploadFinished(true, text);
        } else {
            QString detail = parsedError;
            if (detail.isEmpty()) {
                detail = reply->errorString();
            }
            if (!text.isEmpty()) {
                if (!detail.isEmpty()) {
                    detail += "\n";
                }
                detail += text;
            }
            emit logMessage(QString("Upload failed: %1").arg(detail));
            emit uploadFinished(false, detail);
            emit operationFailed(detail);
        }

        reply->deleteLater();
    });
}

void DeviceClient::rebootDevice()
{
    sendRebootRequest();
}

void DeviceClient::sendRebootRequest()
{
    if (m_deviceHost.isEmpty()) {
        emit operationFailed("Target device address is not set.");
        return;
    }

    QNetworkRequest request(baseUrl().resolved(QUrl("/mplc_reboot.html")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::UserAgentHeader, "MPLCStudio/0.1");

    if (!m_user.isEmpty()) {
        const QByteArray token = QString("%1:%2").arg(m_user, m_password).toUtf8().toBase64();
        request.setRawHeader("Authorization", "Basic " + token);
    }

    emit logMessage(QString("Requesting reboot at %1 ...").arg(request.url().toString()));

    QNetworkReply *reply = m_network->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray body = reply->readAll();
        const bool httpOk = reply->error() == QNetworkReply::NoError;
        const QString text = QString::fromUtf8(body);
        const bool contentOk = text.contains("Rebooting", Qt::CaseInsensitive);

        if (httpOk && contentOk) {
            emit logMessage("Reboot request accepted.");
            emit rebootFinished(true, text);
        } else {
            QString detail = reply->errorString();
            if (!text.isEmpty()) {
                detail += "\n" + text;
            }
            emit logMessage(QString("Reboot failed: %1").arg(detail));
            emit rebootFinished(false, detail);
            emit operationFailed(detail);
        }

        reply->deleteLater();
    });
}
