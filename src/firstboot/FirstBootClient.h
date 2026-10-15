// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QVariantList>

class FirstBootClient final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit FirstBootClient(QObject *parent = nullptr);
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    Q_INVOKABLE bool setLocale(const QString &locale, const QString &keymap, const QString &timezone);
    Q_INVOKABLE bool createUser(const QString &username, const QString &displayName, const QString &password);
    Q_INVOKABLE bool connectWifi(const QString &ssid, const QString &password);
    Q_INVOKABLE bool setPrivacy(bool sshEnabled, bool diagnosticsEnabled);
    Q_INVOKABLE bool complete();

signals:
    void busyChanged();
    void errorChanged();

private:
    bool call(const QString &method, const QVariantList &arguments);
    bool m_busy = false;
    QString m_error;
};

