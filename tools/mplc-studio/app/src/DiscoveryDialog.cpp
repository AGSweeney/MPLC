/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DiscoveryDialog.h"
#include "StudioTheme.h"

#include "DeviceDiscoveryService.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>

DiscoveryDialog::DiscoveryDialog(DeviceDiscoveryService *service, QWidget *parent)
    : QDialog(parent)
    , m_service(service)
{
    setWindowTitle("Discover Devices");
    resize(720, 420);
    setStyleSheet(StudioTheme::dialogStyleSheet() + StudioTheme::styleSheet());

    m_statusLabel = new QLabel("Click Rescan to find NetBurner devices on the LAN.", this);
    m_mplcOnlyCheck = new QCheckBox("Show MPLC targets only", this);
    m_mplcOnlyCheck->setChecked(true);

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({"IP Address", "MAC", "MPLC", "Details"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);

    auto *rescanButton = new QPushButton("Rescan", this);
    m_selectButton = new QPushButton("Use Selected", this);
    m_selectButton->setEnabled(false);
    auto *closeButton = new QPushButton("Close", this);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addWidget(rescanButton);
    buttonRow->addStretch();
    buttonRow->addWidget(m_selectButton);
    buttonRow->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_mplcOnlyCheck);
    layout->addWidget(m_table, 1);
    layout->addLayout(buttonRow);

    connect(rescanButton, &QPushButton::clicked, this, &DiscoveryDialog::startScan);
    connect(m_selectButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &DiscoveryDialog::onSelectionChanged);
    connect(m_mplcOnlyCheck, &QCheckBox::toggled, this, [this]() {
        populateTable(m_devices);
    });

    if (m_service) {
        connect(m_service, &DeviceDiscoveryService::logMessage, this, [this](const QString &message) {
            m_statusLabel->setText(message);
        });
        connect(m_service, &DeviceDiscoveryService::discoveryFinished, this, [this](const QVector<DiscoveredDevice> &devices) {
            m_devices = devices;
            populateTable(m_devices);
            const int mplcCount = std::count_if(m_devices.begin(), m_devices.end(),
                                                [](const DiscoveredDevice &d) { return d.mplcSupported; });
            m_statusLabel->setText(QString("Found %1 NetBurner device(s), %2 MPLC target(s).")
                                       .arg(m_devices.size())
                                       .arg(mplcCount));
        });
    }

    startScan();
}

QString DiscoveryDialog::selectedIp() const
{
    return m_selectedIp;
}

void DiscoveryDialog::populateTable(const QVector<DiscoveredDevice> &devices)
{
    m_table->setRowCount(0);
    m_selectedIp.clear();
    m_selectButton->setEnabled(false);

    for (const DiscoveredDevice &device : devices) {
        if (m_mplcOnlyCheck->isChecked() && !device.mplcSupported) {
            continue;
        }

        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(device.ip));
        m_table->setItem(row, 1, new QTableWidgetItem(device.mac));
        m_table->setItem(row, 2, new QTableWidgetItem(device.mplcSupported ? "Yes" : "No"));
        m_table->setItem(row, 3, new QTableWidgetItem(device.details));
    }

    m_table->resizeColumnsToContents();
}

void DiscoveryDialog::startScan()
{
    if (!m_service) {
        return;
    }

    m_table->setRowCount(0);
    m_selectButton->setEnabled(false);
    m_statusLabel->setText("Scanning...");
    m_service->startDiscovery();
}

void DiscoveryDialog::onSelectionChanged()
{
    const int row = m_table->currentRow();
    if (row < 0) {
        m_selectedIp.clear();
        m_selectButton->setEnabled(false);
        return;
    }

    m_selectedIp = m_table->item(row, 0)->text();
    m_selectButton->setEnabled(!m_selectedIp.isEmpty());
}
