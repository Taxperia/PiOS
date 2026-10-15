// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDBusContext>
#include <QObject>

class FirstBootService final : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.fidan.FirstBoot1")

public:
    explicit FirstBootService(QObject *parent = nullptr);

public slots:
    bool SetLocale(const QString &locale, const QString &keymap, const QString &timezone);
    bool CreateUser(const QString &username, const QString &displayName, const QString &password);
    bool ConnectWifi(const QString &ssid, const QString &password);
    bool SetPrivacy(bool sshEnabled, bool diagnosticsEnabled);
    bool Complete();
    bool IsComplete() const;

signals:
    void Progress(const QString &stage);

private:
    bool callerIsSetupUser() const;
    bool run(const QString &program, const QStringList &arguments, const QByteArray &input = {});
    void fail(const QString &name, const QString &message);
};

