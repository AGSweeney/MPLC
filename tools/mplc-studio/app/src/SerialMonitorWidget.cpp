/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "SerialMonitorWidget.h"
#include "StudioTheme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

#ifdef MPLC_STUDIO_HAS_SERIALPORT
#include <QSerialPort>
#include <QSerialPortInfo>
#endif

SerialMonitorWidget::SerialMonitorWidget(QWidget *parent)
    : QWidget(parent)
{
#ifdef MPLC_STUDIO_HAS_SERIALPORT
    m_port = new QSerialPort(this);
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(20);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(6, 6, 6, 6);
    rootLayout->setSpacing(6);

    auto *toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    auto *portLabel = new QLabel("Port", this);
    portLabel->setObjectName("EndpointLabel");

    m_portCombo = new QComboBox(this);
    m_portCombo->setMinimumWidth(140);
    m_portCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    m_refreshButton = new QPushButton("Refresh", this);
    m_refreshButton->setObjectName("ToolbarButton");
    m_refreshButton->setFixedHeight(28);

    auto *baudLabel = new QLabel("Baud", this);
    baudLabel->setObjectName("EndpointLabel");

    m_baudCombo = new QComboBox(this);
    m_baudCombo->setMinimumWidth(90);
    for (const int baud : {9600, 19200, 38400, 57600, 115200, 230400}) {
        m_baudCombo->addItem(QString::number(baud), baud);
    }
    const int defaultBaudIndex = m_baudCombo->findData(115200);
    if (defaultBaudIndex >= 0) {
        m_baudCombo->setCurrentIndex(defaultBaudIndex);
    }

    m_dtrEnable = new QCheckBox("DTR", this);
    m_dtrEnable->setChecked(true);
    m_dtrEnable->setToolTip("Enable Data Terminal Ready (PuTTY enables this by default).");

    m_rtsEnable = new QCheckBox("RTS", this);
    m_rtsEnable->setChecked(true);
    m_rtsEnable->setToolTip("Enable Request To Send.");

    m_connectButton = new QPushButton("Connect", this);
    m_connectButton->setObjectName("ToolbarButton");
    m_connectButton->setFixedHeight(28);
    m_connectButton->setMinimumWidth(88);

    m_clearButton = new QPushButton("Clear", this);
    m_clearButton->setObjectName("ToolbarButton");
    m_clearButton->setFixedHeight(28);

    toolbar->addWidget(portLabel);
    toolbar->addWidget(m_portCombo);
    toolbar->addWidget(m_refreshButton);
    toolbar->addWidget(baudLabel);
    toolbar->addWidget(m_baudCombo);
    toolbar->addWidget(m_dtrEnable);
    toolbar->addWidget(m_rtsEnable);
    toolbar->addWidget(m_connectButton);
    toolbar->addWidget(m_clearButton);
    toolbar->addStretch(1);

    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setMaximumBlockCount(10000);
    m_output->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_output->setPlaceholderText("Open a serial port to view target output...");
    StudioTheme::styleLogView(m_output);

    auto *sendRow = new QHBoxLayout();
    sendRow->setSpacing(6);

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Type to send (Enter or Send button)");
    m_input->setClearButtonEnabled(true);

    m_appendNewline = new QCheckBox("Append CR+LF", this);
    m_appendNewline->setChecked(true);

    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setObjectName("ToolbarButton");
    m_sendButton->setFixedHeight(28);
    m_sendButton->setEnabled(false);

    sendRow->addWidget(m_input, 1);
    sendRow->addWidget(m_appendNewline);
    sendRow->addWidget(m_sendButton);

    rootLayout->addLayout(toolbar);
    rootLayout->addWidget(m_output, 1);
    rootLayout->addLayout(sendRow);

    connect(m_refreshButton, &QPushButton::clicked, this, &SerialMonitorWidget::refreshPorts);
    connect(m_connectButton, &QPushButton::clicked, this, &SerialMonitorWidget::toggleConnection);
    connect(m_clearButton, &QPushButton::clicked, m_output, &QPlainTextEdit::clear);
    connect(m_sendButton, &QPushButton::clicked, this, &SerialMonitorWidget::sendLine);
    connect(m_port, &QSerialPort::readyRead, this, &SerialMonitorWidget::drainReceive);
    connect(m_pollTimer, &QTimer::timeout, this, &SerialMonitorWidget::drainReceive);
    connect(m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error == QSerialPort::NoError) {
            return;
        }
        if (error == QSerialPort::ResourceError) {
            emit statusMessage(QString("Serial port closed: %1").arg(m_port->errorString()));
            disconnectPort();
            return;
        }
        emit statusMessage(QString("Serial error: %1").arg(m_port->errorString()));
    });

    m_input->installEventFilter(this);

    refreshPorts();
#else
    buildUnavailableUi(
        "The Qt SerialPort module is not available in this build.\n\n"
        "Install it with the Qt Maintenance Tool (Qt Serial Bus → Qt SerialPort), then rebuild MPLC Studio.");
#endif
}

void SerialMonitorWidget::buildUnavailableUi(const QString &detail)
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);

    auto *title = new QLabel("Serial monitor unavailable", this);
    title->setObjectName("EndpointLabel");

    auto *body = new QLabel(detail, this);
    body->setWordWrap(true);

    rootLayout->addWidget(title);
    rootLayout->addWidget(body);
    rootLayout->addStretch(1);
}

bool SerialMonitorWidget::isConnected() const
{
#ifdef MPLC_STUDIO_HAS_SERIALPORT
    return m_port != nullptr && m_port->isOpen();
#else
    return false;
#endif
}

int SerialMonitorWidget::selectedBaudRate() const
{
    if (m_baudCombo == nullptr) {
        return 115200;
    }
    const int baud = m_baudCombo->currentData().toInt();
    return baud > 0 ? baud : 115200;
}

void SerialMonitorWidget::disconnectPort()
{
#ifdef MPLC_STUDIO_HAS_SERIALPORT
    if (m_pollTimer != nullptr) {
        m_pollTimer->stop();
    }
    if (m_port == nullptr || !m_port->isOpen()) {
        return;
    }

    m_port->close();
    m_connectButton->setText("Connect");
    m_portCombo->setEnabled(true);
    m_baudCombo->setEnabled(true);
    m_dtrEnable->setEnabled(true);
    m_rtsEnable->setEnabled(true);
    m_refreshButton->setEnabled(true);
    m_sendButton->setEnabled(false);
    emit statusMessage("Serial monitor disconnected.");
#endif
}

void SerialMonitorWidget::refreshPorts()
{
#ifdef MPLC_STUDIO_HAS_SERIALPORT
    const QString previous = m_portCombo->currentData().toString();
    m_portCombo->clear();

    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        QString label = info.portName();
        if (!info.description().isEmpty()) {
            label += " — " + info.description();
        }
        m_portCombo->addItem(label, info.portName());
    }

    if (m_portCombo->count() == 0) {
        m_portCombo->addItem("No ports found", QString());
        m_connectButton->setEnabled(false);
        return;
    }

    m_connectButton->setEnabled(true);
    const int index = m_portCombo->findData(previous);
    if (index >= 0) {
        m_portCombo->setCurrentIndex(index);
    }
#endif
}

void SerialMonitorWidget::toggleConnection()
{
#ifdef MPLC_STUDIO_HAS_SERIALPORT
    if (m_port->isOpen()) {
        disconnectPort();
        return;
    }

    const QString portName = m_portCombo->currentData().toString();
    if (portName.isEmpty()) {
        QMessageBox::warning(this, "Serial Monitor", "No serial port selected.");
        return;
    }

    m_port->setPortName(portName);
    m_port->setBaudRate(selectedBaudRate());
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port->open(QIODevice::ReadWrite)) {
        QMessageBox::warning(this, "Serial Monitor",
                             QString("Could not open %1:\n%2\n\nClose PuTTY or any other app using this port first.")
                                 .arg(portName, m_port->errorString()));
        return;
    }

    m_port->clear(QSerialPort::AllDirections);
    m_port->setDataTerminalReady(m_dtrEnable->isChecked());
    m_port->setRequestToSend(m_rtsEnable->isChecked());

    m_connectButton->setText("Disconnect");
    m_portCombo->setEnabled(false);
    m_baudCombo->setEnabled(false);
    m_dtrEnable->setEnabled(false);
    m_rtsEnable->setEnabled(false);
    m_refreshButton->setEnabled(false);
    m_sendButton->setEnabled(true);

    drainReceive();
    m_pollTimer->start();

    emit statusMessage(QString("Serial monitor connected on %1 at %2 baud (DTR=%3, RTS=%4).")
                           .arg(portName)
                           .arg(selectedBaudRate())
                           .arg(m_dtrEnable->isChecked() ? "on" : "off")
                           .arg(m_rtsEnable->isChecked() ? "on" : "off"));
#endif
}

void SerialMonitorWidget::drainReceive()
{
#ifdef MPLC_STUDIO_HAS_SERIALPORT
    if (m_port == nullptr || !m_port->isOpen()) {
        return;
    }

    const QByteArray data = m_port->readAll();
    if (!data.isEmpty()) {
        appendReceived(data);
    }
#endif
}

void SerialMonitorWidget::sendLine()
{
#ifdef MPLC_STUDIO_HAS_SERIALPORT
    if (!m_port->isOpen()) {
        return;
    }

    QByteArray payload = m_input->text().toLatin1();
    if (payload.isEmpty()) {
        return;
    }
    if (m_appendNewline->isChecked()) {
        payload.append("\r\n");
    }

    if (m_port->write(payload) != payload.size()) {
        QMessageBox::warning(this, "Serial Monitor", m_port->errorString());
        return;
    }
    m_port->flush();

    m_input->clear();
#endif
}

void SerialMonitorWidget::appendReceived(const QByteArray &data)
{
    if (data.isEmpty() || m_output == nullptr) {
        return;
    }

    const QString text = QString::fromLatin1(data);
    m_output->moveCursor(QTextCursor::End);
    m_output->insertPlainText(text);
    m_output->ensureCursorVisible();
    if (QScrollBar *bar = m_output->verticalScrollBar()) {
        bar->setValue(bar->maximum());
    }
}

bool SerialMonitorWidget::eventFilter(QObject *watched, QEvent *event)
{
#ifdef MPLC_STUDIO_HAS_SERIALPORT
    if (watched == m_input && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            sendLine();
            return true;
        }
    }
#endif
    return QWidget::eventFilter(watched, event);
}

void SerialMonitorWidget::loadSettings()
{
#ifdef MPLC_STUDIO_HAS_SERIALPORT
    QSettings settings;
    const QString savedPort = settings.value("serial/port").toString();
    const int savedBaud = settings.value("serial/baud", 115200).toInt();
    const bool appendNewline = settings.value("serial/appendNewline", true).toBool();
    const bool dtr = settings.value("serial/dtr", true).toBool();
    const bool rts = settings.value("serial/rts", true).toBool();

    refreshPorts();
    if (!savedPort.isEmpty()) {
        const int portIndex = m_portCombo->findData(savedPort);
        if (portIndex >= 0) {
            m_portCombo->setCurrentIndex(portIndex);
        }
    }

    const int baudIndex = m_baudCombo->findData(savedBaud);
    if (baudIndex >= 0) {
        m_baudCombo->setCurrentIndex(baudIndex);
    }

    m_appendNewline->setChecked(appendNewline);
    m_dtrEnable->setChecked(dtr);
    m_rtsEnable->setChecked(rts);
#endif
}

void SerialMonitorWidget::saveSettings()
{
#ifdef MPLC_STUDIO_HAS_SERIALPORT
    QSettings settings;
    settings.setValue("serial/port", m_portCombo->currentData().toString());
    settings.setValue("serial/baud", selectedBaudRate());
    settings.setValue("serial/appendNewline", m_appendNewline->isChecked());
    settings.setValue("serial/dtr", m_dtrEnable->isChecked());
    settings.setValue("serial/rts", m_rtsEnable->isChecked());
#endif
}
