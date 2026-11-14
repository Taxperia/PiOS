// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>
#include <QDBusConnection>

class FirstBootClient final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QVariantMap state READ state NOTIFY stateChanged)
    Q_PROPERTY(QStringList networks READ networks NOTIFY networksChanged)

public:
    explicit FirstBootClient(QObject *parent = nullptr);
    FirstBootClient(const QDBusConnection &bus, QObject *parent = nullptr);
    bool busy() const { return m_busy || m_state.value("updateBusy").toBool(); }
    QString error() const { return m_error; }
    Q_INVOKABLE bool setLocale(const QString &locale, const QString &keymap, const QString &timezone);
    Q_INVOKABLE bool createUser(const QString &username, const QString &displayName, const QString &password);
    Q_INVOKABLE bool connectWifi(const QString &ssid, const QString &password);
    Q_INVOKABLE bool setPrivacy(bool sshEnabled, bool diagnosticsEnabled, bool remoteEnabled = false);
    Q_INVOKABLE bool complete();
    Q_INVOKABLE bool setDesktopPreferences(const QVariantMap &preferences);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void scanWifi();
    Q_INVOKABLE bool skipNetwork();
    Q_INVOKABLE bool startUpdate(const QString &action);
    Q_INVOKABLE bool skipUpdates();
    Q_INVOKABLE void setLanguage(const QString &locale);
    QVariantMap state() const { return m_state; }
    QStringList networks() const { return m_networks; }

signals:
    void busyChanged();
    void errorChanged();
    void stateChanged();
    void networksChanged();
    void operationFinished(const QString &operation, bool success);
    void languageChanged(const QString &locale);

private:
    bool call(const QString &method, const QVariantList &arguments);
    bool m_busy = false;
    QString m_error;
    QVariantMap m_state;
    QStringList m_networks;
    QTimer m_poll;
    bool m_refreshing = false;
    QDBusConnection m_bus;
};

