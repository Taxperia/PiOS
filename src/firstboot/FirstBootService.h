// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDBusContext>
#include <QObject>
#include <QVariantMap>
#include <QProcess>
#include <QTimer>
#include <functional>
#include <QFile>

class FirstBootService final : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.fidan.FirstBoot1")

public:
    struct Result { bool success; QByteArray output; };
    using Runner = std::function<Result(const QString &, const QStringList &, const QByteArray &)>;
    explicit FirstBootService(QObject *parent = nullptr);
    FirstBootService(const QString &systemRoot, Runner runner, std::function<bool()> authorize, QObject *parent = nullptr);

public slots:
    bool SetLocale(const QString &locale, const QString &keymap, const QString &timezone);
    bool CreateUser(const QString &username, const QString &displayName, const QString &password);
    bool ConnectWifi(const QString &ssid, const QString &password);
    bool SetPrivacy(bool sshEnabled, bool diagnosticsEnabled, bool remoteEnabled = false);
    bool SetDesktopPreferences(const QVariantMap &preferences);
    bool Complete();
    bool IsComplete() const;
    QVariantMap State() const;
    QStringList ScanWifi();
    bool SkipNetwork();
    bool StartUpdate(const QString &action);
    bool SkipUpdates();

signals:
    void Progress(const QString &stage);

private:
    bool callerIsSetupUser() const;
    bool updateRunning() const;
    bool run(const QString &program, const QStringList &arguments, const QByteArray &input = {});
    bool ensureAccountHome(const QString &username);
    Result execute(const QString &program, const QStringList &arguments, const QByteArray &input = {});
    QString path(const QString &absolute) const;
    bool saveState();
    bool writeConfig(const QString &name, const QByteArray &data, QFileDevice::Permissions permissions);
    void fail(const QString &name, const QString &message);
    QString m_root;
    Runner m_runner;
    std::function<bool()> m_authorize;
    QVariantMap m_state;
    bool m_validState = true;
    QProcess m_update;
    QTimer m_updateTimeout;
    QString m_updateAction, m_updateError;
    QVariantMap m_updateResult;
};

