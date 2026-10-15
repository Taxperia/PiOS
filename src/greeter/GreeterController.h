// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QLocalSocket>

class QJsonObject;

class GreeterController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString prompt READ prompt NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)

public:
    explicit GreeterController(QObject *parent = nullptr);
    bool busy() const { return m_busy; }
    QString prompt() const { return m_prompt; }
    QString error() const { return m_error; }

    Q_INVOKABLE void beginLogin(const QString &username);
    Q_INVOKABLE void answer(const QString &response);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void powerOff();
    Q_INVOKABLE void reboot();

signals:
    void stateChanged();
    void authenticated();

private slots:
    void readResponse();
    void socketFailed(QLocalSocket::LocalSocketError error);

private:
    void send(const QJsonObject &message);
    void setError(const QString &message);
    QLocalSocket m_socket;
    QByteArray m_buffer;
    bool m_busy = false;
    QString m_prompt;
    QString m_error;
};

