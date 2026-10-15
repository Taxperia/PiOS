// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QVariantList>
#include <QProcess>
#include <QTimer>

class SettingsData final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap info READ info NOTIFY systemChanged)
    Q_PROPERTY(QVariantList volumes READ volumes NOTIFY systemChanged)
    Q_PROPERTY(QVariantList wallpapers READ wallpapers CONSTANT)
    Q_PROPERTY(QString licenseText READ licenseText CONSTANT)
    Q_PROPERTY(QVariantList networks READ networks NOTIFY networkChanged)
    Q_PROPERTY(QVariantMap connection READ connection NOTIFY networkChanged)
    Q_PROPERTY(bool wifiAvailable READ wifiAvailable NOTIFY networkChanged)
    Q_PROPERTY(bool networkBusy READ networkBusy NOTIFY networkChanged)
    Q_PROPERTY(QString networkError READ networkError NOTIFY networkChanged)
    Q_PROPERTY(bool actionBusy READ actionBusy NOTIFY actionChanged)
    Q_PROPERTY(QString actionError READ actionError NOTIFY actionChanged)
public:
    explicit SettingsData(bool mock = false, QObject *parent = nullptr);
    ~SettingsData() override;
    QVariantMap info() const { return m_info; }
    QVariantList volumes() const { return m_volumes; }
    QVariantList wallpapers() const;
    QString licenseText() const;
    QVariantList networks() const { return m_networks; }
    QVariantMap connection() const { return m_connection; }
    bool wifiAvailable() const { return !m_wifiDevice.isEmpty(); }
    bool networkBusy() const { return m_network.state() != QProcess::NotRunning; }
    QString networkError() const { return m_networkError; }
    bool actionBusy() const { return m_action.state() != QProcess::NotRunning; }
    QString actionError() const { return m_actionError; }
    Q_INVOKABLE void refreshSystem();
    Q_INVOKABLE void refreshNetwork(bool rescan = false);
    Q_INVOKABLE void connectNetwork(const QString &ssid, const QString &password);
    Q_INVOKABLE void disconnectNetwork();
    Q_INVOKABLE void setNtp(bool enabled);
    Q_INVOKABLE bool toolAvailable(const QString &key) const;
    Q_INVOKABLE bool openTool(const QString &key);
    Q_INVOKABLE bool openHelp();
    static QStringList splitFields(const QString &line);
signals:
    void systemChanged();
    void networkChanged();
    void actionChanged();
private:
    void networkStart(const QString &phase, const QStringList &args);
    void networkFinished(int code, QProcess::ExitStatus status);
    void queryConnection();
    bool m_mock = false, m_rescan = false;
    QVariantMap m_info, m_connection;
    QVariantList m_volumes, m_networks;
    QString m_wifiDevice, m_connectionDevice, m_phase, m_networkError, m_actionError;
    QProcess m_network, m_action;
    QTimer m_networkTimeout, m_actionTimeout;
};
