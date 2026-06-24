/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "DiscoveredDevice.h"

#include <QDialog>
#include <QVector>

class QCheckBox;
class QLabel;
class QPushButton;
class QTableWidget;

class DeviceDiscoveryService;

class DiscoveryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DiscoveryDialog(DeviceDiscoveryService *service, QWidget *parent = nullptr);

    QString selectedIp() const;

private:
    void populateTable(const QVector<DiscoveredDevice> &devices);
    void startScan();
    void onSelectionChanged();

    DeviceDiscoveryService *m_service = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_statusLabel = nullptr;
    QCheckBox *m_mplcOnlyCheck = nullptr;
    QPushButton *m_selectButton = nullptr;
    QVector<DiscoveredDevice> m_devices;
    QString m_selectedIp;
};
