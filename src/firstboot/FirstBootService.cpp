// SPDX-License-Identifier: GPL-3.0-or-later
#include "FirstBootService.h"
#include "FirstBootValidation.h"

#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>

#include <pwd.h>

namespace {
const QString completeMarker = QStringLiteral("/var/lib/fidan/firstboot-complete");
}

FirstBootService::FirstBootService(QObject *parent)
    : QObject(parent)
{
}

bool FirstBootService::callerIsSetupUser() const
{
    // An existing setup process must lose provisioning rights after completion.
    if (QFileInfo::exists(completeMarker) || !calledFromDBus()) {
        return false;
    }
    const QDBusReply<uint> uid = connection().interface()->serviceUid(message().service());
    if (!uid.isValid() || uid.value() == 0) {
        return uid.isValid();
    }
    const passwd *account = getpwuid(static_cast<uid_t>(uid.value()));
    return account && QByteArray(account->pw_name) == QByteArrayLiteral("fidan-setup");
}

void FirstBootService::fail(const QString &name, const QString &messageText)
{
    if (calledFromDBus()) {
        sendErrorReply(name, messageText);
    }
}

bool FirstBootService::run(const QString &program, const QStringList &arguments, const QByteArray &input)
{
    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start();
    if (!process.waitForStarted(5000)) {
        return false;
    }
    if (!input.isEmpty()) {
        process.write(input);
    }
    process.closeWriteChannel();
    return process.waitForFinished(30000) && process.exitStatus() == QProcess::NormalExit
        && process.exitCode() == 0;
}

bool FirstBootService::SetLocale(const QString &locale, const QString &keymap, const QString &timezone)
{
    if (!callerIsSetupUser()) {
        fail(QStringLiteral("org.fidan.Error.AccessDenied"), QStringLiteral("The caller is not the local setup session."));
        return false;
    }
    static const QRegularExpression localePattern(QStringLiteral("\\A[A-Za-z]{2}_[A-Za-z]{2}\\.UTF-8\\z"));
    static const QRegularExpression keymapPattern(QStringLiteral("\\A[A-Za-z0-9][A-Za-z0-9_-]{1,23}\\z"));
    static const QRegularExpression zonePattern(QStringLiteral("\\A[A-Za-z0-9_+-]+(?:/[A-Za-z0-9_+-]+)*\\z"));
    const QFileInfo zone(QStringLiteral("/usr/share/zoneinfo/") + timezone);
    const QString zoneRoot = QFileInfo(QStringLiteral("/usr/share/zoneinfo")).canonicalFilePath();
    if (!localePattern.match(locale).hasMatch() || !keymapPattern.match(keymap).hasMatch()
        || !zonePattern.match(timezone).hasMatch() || !zone.isFile() || zoneRoot.isEmpty()
        || !zone.canonicalFilePath().startsWith(zoneRoot + QLatin1Char('/'))) {
        fail(QStringLiteral("org.fidan.Error.InvalidInput"), QStringLiteral("Locale, keymap, or timezone is invalid."));
        return false;
    }

    emit Progress(QStringLiteral("locale"));
    return run(QStringLiteral("/usr/bin/localectl"), {QStringLiteral("set-locale"), QStringLiteral("LANG=") + locale})
        && run(QStringLiteral("/usr/bin/localectl"), {QStringLiteral("set-keymap"), keymap})
        && run(QStringLiteral("/usr/bin/timedatectl"), {QStringLiteral("set-timezone"), timezone});
}

bool FirstBootService::CreateUser(const QString &username, const QString &displayName, const QString &password)
{
    if (!callerIsSetupUser()) {
        fail(QStringLiteral("org.fidan.Error.AccessDenied"), QStringLiteral("The caller is not the local setup session."));
        return false;
    }
    if (!FirstBootValidation::account(username, displayName, password)) {
        fail(QStringLiteral("org.fidan.Error.InvalidInput"), QStringLiteral("The account information is invalid."));
        return false;
    }

    emit Progress(QStringLiteral("account"));
    if (!run(QStringLiteral("/usr/sbin/useradd"),
             {QStringLiteral("--create-home"), QStringLiteral("--shell"), QStringLiteral("/bin/bash"),
              QStringLiteral("--comment"), displayName, QStringLiteral("--groups"),
              QStringLiteral("sudo,adm,audio,video,render,input,plugdev,netdev,gpio,i2c,spi"), username})) {
        fail(QStringLiteral("org.fidan.Error.AccountCreationFailed"), QStringLiteral("The user account could not be created."));
        return false;
    }

    const QByteArray passwordInput = username.toUtf8() + ':' + password.toUtf8() + '\n';
    if (!run(QStringLiteral("/usr/sbin/chpasswd"), {}, passwordInput)) {
        fail(QStringLiteral("org.fidan.Error.PasswordFailed"), QStringLiteral("The password could not be set."));
        return false;
    }
    return true;
}

bool FirstBootService::ConnectWifi(const QString &ssid, const QString &password)
{
    if (!callerIsSetupUser() || ssid.isEmpty() || ssid.toUtf8().size() > 32 || password.size() > 63
        || !FirstBootValidation::singleLine(ssid) || !FirstBootValidation::singleLine(password)) {
        fail(QStringLiteral("org.fidan.Error.InvalidInput"), QStringLiteral("The Wi-Fi settings are invalid."));
        return false;
    }

    emit Progress(QStringLiteral("network"));
    QProcess process;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    process.setProcessEnvironment(environment);
    process.setProgram(QStringLiteral("/usr/bin/nmcli"));
    process.setArguments({QStringLiteral("--ask"), QStringLiteral("device"), QStringLiteral("wifi"),
                          QStringLiteral("connect"), ssid});
    process.start();
    if (!process.waitForStarted(5000)) {
        return false;
    }
    process.write(password.toUtf8() + '\n');
    process.closeWriteChannel();
    return process.waitForFinished(45000) && process.exitCode() == 0;
}

bool FirstBootService::SetPrivacy(bool sshEnabled, bool diagnosticsEnabled)
{
    if (!callerIsSetupUser()) {
        fail(QStringLiteral("org.fidan.Error.AccessDenied"), QStringLiteral("The caller is not the local setup session."));
        return false;
    }

    emit Progress(QStringLiteral("privacy"));
    QDir().mkpath(QStringLiteral("/etc/fidan"));
    QSaveFile privacy(QStringLiteral("/etc/fidan/privacy.conf"));
    if (!privacy.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fail(QStringLiteral("org.fidan.Error.WriteFailed"), QStringLiteral("Privacy settings could not be saved."));
        return false;
    }
    privacy.write("[Privacy]\nDiagnosticsEnabled=");
    privacy.write(diagnosticsEnabled ? "true\n" : "false\n");
    privacy.write("SshEnabled=");
    privacy.write(sshEnabled ? "true\n" : "false\n");
    if (!privacy.commit()) {
        fail(QStringLiteral("org.fidan.Error.WriteFailed"), QStringLiteral("Privacy settings could not be saved."));
        return false;
    }

    return run(QStringLiteral("/usr/bin/systemctl"),
               {sshEnabled ? QStringLiteral("enable") : QStringLiteral("disable"),
                QStringLiteral("ssh.service")});
}

bool FirstBootService::Complete()
{
    if (!callerIsSetupUser()) {
        fail(QStringLiteral("org.fidan.Error.AccessDenied"), QStringLiteral("The caller is not the local setup session."));
        return false;
    }
    QDir().mkpath(QFileInfo(completeMarker).absolutePath());
    QFile marker(completeMarker);
    if (!marker.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    marker.write("complete\n");
    marker.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                          | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    marker.close();
    emit Progress(QStringLiteral("complete"));
    run(QStringLiteral("/usr/bin/systemctl"), {QStringLiteral("enable"), QStringLiteral("greetd.service")});
    run(QStringLiteral("/usr/bin/systemctl"), {QStringLiteral("disable"), QStringLiteral("fidan-firstboot-session.service")});
    QProcess::startDetached(QStringLiteral("/usr/bin/systemd-run"),
                            {QStringLiteral("--unit=fidan-firstboot-reboot"),
                             QStringLiteral("--on-active=2s"),
                             QStringLiteral("/usr/bin/systemctl"), QStringLiteral("reboot")});
    return true;
}

bool FirstBootService::IsComplete() const
{
    return QFileInfo::exists(completeMarker);
}

