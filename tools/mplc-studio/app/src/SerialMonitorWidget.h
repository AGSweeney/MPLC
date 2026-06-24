/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QWidget>

class QComboBox;
class QCheckBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTimer;

#ifdef MPLC_STUDIO_HAS_SERIALPORT
class QSerialPort;
#endif

class SerialMonitorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SerialMonitorWidget(QWidget *parent = nullptr);

    void loadSettings();
    void saveSettings();
    bool isConnected() const;
    void disconnectPort();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void statusMessage(const QString &message);

private:
    void refreshPorts();
    void toggleConnection();
    void drainReceive();
    void sendLine();
    void appendReceived(const QByteArray &data);
    void buildUnavailableUi(const QString &detail);
    int selectedBaudRate() const;

    QComboBox *m_portCombo = nullptr;
    QComboBox *m_baudCombo = nullptr;
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPlainTextEdit *m_output = nullptr;
    QLineEdit *m_input = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QCheckBox *m_appendNewline = nullptr;
    QCheckBox *m_dtrEnable = nullptr;
    QCheckBox *m_rtsEnable = nullptr;
    QTimer *m_pollTimer = nullptr;

#ifdef MPLC_STUDIO_HAS_SERIALPORT
    QSerialPort *m_port = nullptr;
#endif
};
