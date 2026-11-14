// SPDX-License-Identifier: GPL-3.0-or-later
#include "FirstBootService.h"
#include "FirstBootValidation.h"
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <pwd.h>

FirstBootService::FirstBootService(QObject *parent) : FirstBootService({}, {}, {}, parent) {}

FirstBootService::FirstBootService(const QString &root, Runner runner, std::function<bool()> authorize, QObject *parent)
    : QObject(parent), m_root(root), m_runner(std::move(runner)), m_authorize(std::move(authorize))
{
    QFile state(path("/var/lib/fidan/setup-state.json"));
    if (state.exists()) {
        QJsonParseError error;
        if (!state.open(QIODevice::ReadOnly) || state.size() > 16384) m_validState = false;
        else {
            const auto json = QJsonDocument::fromJson(state.readAll(), &error);
            m_validState = error.error == QJsonParseError::NoError && json.isObject() && json.object().value("schema").toInt() == 1;
            if (m_validState) m_state = json.object().toVariantMap();
        }
    }
    m_updateTimeout.setSingleShot(true);
    m_updateTimeout.setInterval(30 * 60 * 1000);
    connect(&m_updateTimeout, &QTimer::timeout, this, [this] { m_updateError = tr("The update timed out. You can retry or continue offline."); m_update.kill(); });
    connect(&m_update, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) { m_updateTimeout.stop(); m_updateError = tr("The update service could not be started."); }
    });
    connect(&m_update, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this](int code, QProcess::ExitStatus status) {
        m_updateTimeout.stop();
        const auto output = m_update.readAllStandardOutput();
        const QString error = QString::fromUtf8(m_update.readAllStandardError()).right(2048).trimmed();
        if (status != QProcess::NormalExit || code != 0) {
            if (m_updateError.isEmpty()) m_updateError = error.isEmpty() ? tr("The update failed. Retry or continue without updating.") : error;
            return;
        }
        if (m_updateAction == "install") {
            m_state["updateDone"] = true;
            m_updateResult["installed"] = true;
            if (!saveState()) m_updateError = tr("The setup progress could not be saved.");
            return;
        }
        QJsonParseError parse;
        const auto json = QJsonDocument::fromJson(output.trimmed(), &parse);
        if (parse.error != QJsonParseError::NoError || !json.isObject()) { m_updateError = tr("The update service returned an invalid response."); return; }
        m_updateResult = json.object().toVariantMap();
        if ((m_updateAction == "check" && !m_updateResult.value("updateAvailable").toBool())
            || (m_updateAction == "download" && !m_updateResult.value("downloaded").toBool())) {
            m_state["updateDone"] = true;
            if (!saveState()) m_updateError = tr("The setup progress could not be saved.");
        }
    });
}

QString FirstBootService::path(const QString &absolute) const { return m_root + absolute; }

bool FirstBootService::callerIsSetupUser() const
{
    if (IsComplete() || !m_validState) return false;
    if (m_authorize) return m_authorize();
    if (!calledFromDBus()) return false;
    const QDBusReply<uint> uid = connection().interface()->serviceUid(message().service());
    if (!uid.isValid()) return false;
    if (uid.value() == 0) return true;
    const passwd *account = getpwuid(uid.value());
    return account && QByteArray(account->pw_name) == "fidan-setup";
}

void FirstBootService::fail(const QString &name, const QString &message)
{
    if (calledFromDBus()) sendErrorReply(name, message);
}

FirstBootService::Result FirstBootService::execute(const QString &program, const QStringList &arguments, const QByteArray &input)
{
    if (m_runner) return m_runner(program, arguments, input);
    QProcess process;
    auto environment = QProcessEnvironment::systemEnvironment(); environment.insert("LC_ALL", "C");
    process.setProcessEnvironment(environment);
    process.start(program, arguments);
    if (!process.waitForStarted(5000)) return {false, {}};
    if (!input.isEmpty()) process.write(input);
    process.closeWriteChannel();
    if (!process.waitForFinished(60000)) { process.kill(); process.waitForFinished(5000); return {false, {}}; }
    return {process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0, process.readAllStandardOutput().left(65536)};
}
bool FirstBootService::run(const QString &program, const QStringList &arguments, const QByteArray &input) { return execute(program, arguments, input).success; }

bool FirstBootService::ensureAccountHome(const QString &username)
{
    // useradd can reserve the account before failing to create its home. A
    // resumed password step must repair that partial account before marking it ready.
    return run("/usr/sbin/mkhomedir_helper", {username, "0077"})
        && run("/usr/bin/test", {"-d", path("/home/") + username});
}

bool FirstBootService::writeConfig(const QString &name, const QByteArray &data, QFileDevice::Permissions permissions)
{
    if (!QDir().mkpath(QFileInfo(path(name)).absolutePath())) return false;
    QSaveFile file(path(name));
    if (!file.open(QIODevice::WriteOnly)) return false;
    if (!file.setPermissions(permissions) || file.write(data) != data.size() || !file.commit()) return false;
    return true;
}
bool FirstBootService::saveState()
{
    m_state["schema"] = 1;
    return writeConfig("/var/lib/fidan/setup-state.json", QJsonDocument::fromVariant(m_state).toJson(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

QVariantMap FirstBootService::State() const
{
    if (IsComplete()) return {{"complete", true}};
    if (!callerIsSetupUser()) return {{"error", tr("Setup is unavailable or its saved progress needs repair.")}};
    auto state = m_state;
    state["step"] = !state.value("localeReady").toBool() ? 1 : !state.value("accountReady").toBool() ? 2
        : !state.value("networkDone").toBool() ? 3 : !state.value("privacyDone").toBool() ? 4 : !state.value("updateDone").toBool() ? 5 : 6;
    state["updateBusy"] = updateRunning();
    state["updateAction"] = m_updateAction;
    state["updateError"] = m_updateError;
    state["update"] = m_updateResult;
    return state;
}

bool FirstBootService::updateRunning() const
{
    if (m_update.state() != QProcess::NotRunning) return true;
    if (m_runner) return false;
    // A transient APT service can outlive this daemon or its waiting client.
    // Never allow completion/reboot while that service is still active.
    QDBusInterface manager("org.freedesktop.systemd1", "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager", QDBusConnection::systemBus());
    QDBusReply<QDBusObjectPath> unit = manager.call("GetUnit", "fidan-firstboot-update.service");
    if (!unit.isValid()) return false;
    QDBusInterface service("org.freedesktop.systemd1", unit.value().path(), "org.freedesktop.systemd1.Unit", QDBusConnection::systemBus());
    return QStringList{"active", "activating", "deactivating", "reloading"}.contains(service.property("ActiveState").toString());
}

bool FirstBootService::SetLocale(const QString &locale, const QString &keymap, const QString &timezone)
{
    if (!callerIsSetupUser()) return false;
    const QFileInfo zone(path("/usr/share/zoneinfo/") + timezone);
    const QString zoneRoot = QFileInfo(path("/usr/share/zoneinfo")).canonicalFilePath();
    if (!QStringList{"tr_TR.UTF-8", "en_US.UTF-8"}.contains(locale) || !QStringList{"trq", "us"}.contains(keymap)
        || !FirstBootValidation::singleLine(timezone) || !zone.isFile() || zoneRoot.isEmpty()
        || !zone.canonicalFilePath().startsWith(zoneRoot + '/')) {
        fail("org.fidan.Error.InvalidInput", tr("Choose a supported language, keyboard and time zone.")); return false;
    }
    const QByteArray layout = keymap == "trq" ? "tr" : "us";
    const auto mode = QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther;
    // Debian's localectl does not implement setting console/X11 keymaps.
    if (!writeConfig("/etc/default/locale", "LANG=" + locale.toUtf8() + '\n', mode)
        || !writeConfig("/etc/locale.conf", "LANG=" + locale.toUtf8() + '\n', mode)
        || !writeConfig("/etc/default/keyboard", "XKBMODEL=\"pc105\"\nXKBLAYOUT=\"" + layout + "\"\nXKBVARIANT=\"\"\nXKBOPTIONS=\"\"\n", mode)
        || !writeConfig("/etc/vconsole.conf", "KEYMAP=" + keymap.toUtf8() + '\n', mode)
        || !run("/usr/bin/timedatectl", {"set-timezone", timezone})) return false;
    m_state["locale"] = locale; m_state["keymap"] = keymap; m_state["timezone"] = timezone; m_state["localeReady"] = true;
    emit Progress("locale"); return saveState();
}

bool FirstBootService::CreateUser(const QString &username, const QString &displayName, const QString &password)
{
    if (!callerIsSetupUser() || !m_state.value("localeReady").toBool()) return false;
    if (!FirstBootValidation::account(username, displayName, password)) { fail("org.fidan.Error.InvalidInput", tr("Check the account details and use a password of at least eight characters.")); return false; }
    const QString managed = m_state.value("username").toString();
    const bool exists = run("/usr/bin/getent", {"passwd", username});
    if ((!managed.isEmpty() && managed != username) || (exists && managed != username)) {
        fail("org.fidan.Error.AccountExists", tr("This account already exists. Choose another name; a setup account already created cannot be renamed here.")); return false;
    }
    m_state["username"] = username;
    if (!saveState()) return false; // Own only this newly reserved account across retries.
    if (!exists) {
        QStringList groups;
        for (const auto &group : {"sudo", "adm", "audio", "video", "render", "input", "plugdev", "netdev", "gpio", "i2c", "spi"})
            if (run("/usr/bin/getent", {"group", group})) groups.append(group);
        if (!groups.contains("sudo") || !run("/usr/sbin/useradd", {"--create-home", "--home-dir", "/home/" + username, "--shell", "/bin/bash", "--comment", displayName, "--groups", groups.join(','), username})) return false;
    }
    if (!ensureAccountHome(username)) {
        fail("org.fidan.Error.HomeUnavailable", tr("The account home could not be created. Retry this step before continuing.")); return false;
    }
    if (!run("/usr/sbin/chpasswd", {}, username.toUtf8() + ':' + password.toUtf8() + '\n')
        || !run("/usr/sbin/usermod", {"--comment", displayName, username})) {
        fail("org.fidan.Error.PasswordFailed", tr("The account is reserved. Retry setting its password to finish this step.")); return false;
    }
    m_state["displayName"] = displayName; m_state["accountReady"] = true;
    emit Progress("account"); return saveState();
}

QStringList FirstBootService::ScanWifi()
{
    if (!callerIsSetupUser()) return {};
    const auto result = execute("/usr/bin/nmcli", {"--wait", "12", "--escape", "no", "--get-values", "SSID", "device", "wifi", "list", "--rescan", "yes"});
    if (!result.success) { fail("org.fidan.Error.WifiScan", tr("No Wi-Fi adapter or network was found. You can enter a hidden network or continue offline.")); return {}; }
    QStringList networks;
    for (const auto &line : QString::fromUtf8(result.output).split('\n'))
        if (!line.isEmpty() && line.toUtf8().size() <= 32 && FirstBootValidation::singleLine(line) && !networks.contains(line)) networks.append(line);
    return networks;
}
bool FirstBootService::ConnectWifi(const QString &ssid, const QString &password)
{
    if (!callerIsSetupUser() || !m_state.value("accountReady").toBool() || ssid.isEmpty() || ssid.toUtf8().size() > 32 || password.size() > 63
        || !FirstBootValidation::singleLine(ssid) || !FirstBootValidation::singleLine(password)) return false;
    QStringList arguments{"--wait", "40", "--ask", "device", "wifi", "connect", ssid};
    const auto visible = execute("/usr/bin/nmcli", {"--escape", "no", "--get-values", "SSID", "device", "wifi", "list", "--rescan", "no"});
    if (visible.success && !visible.output.split('\n').contains(ssid.toUtf8())) arguments << "hidden" << "yes";
    if (!run("/usr/bin/nmcli", arguments, password.toUtf8() + '\n')) {
        fail("org.fidan.Error.WifiConnection", tr("Could not connect. Check the network name and password, retry, or continue offline.")); return false;
    }
    m_state["networkDone"] = true; m_state["ssid"] = ssid;
    emit Progress("network"); return saveState();
}
bool FirstBootService::SkipNetwork()
{
    if (!callerIsSetupUser() || !m_state.value("accountReady").toBool()) return false;
    m_state["networkDone"] = true; return saveState();
}
bool FirstBootService::SetPrivacy(bool ssh, bool diagnostics, bool remote)
{
    if (!callerIsSetupUser() || !m_state.value("accountReady").toBool()) return false;
    // There is no diagnostic collection service. Do not record consent for a
    // future implementation through an old or custom D-Bus client.
    if (diagnostics) { fail("org.fidan.Error.Unavailable", tr("Diagnostic sharing is not available.")); return false; }
    if (!run("/usr/bin/systemctl", {ssh ? "enable" : "disable", "ssh.service"})) return false;
    if (!writeConfig("/etc/fidan/privacy.conf", QByteArray("[Privacy]\nDiagnosticsEnabled=") + (diagnostics ? "true" : "false")
        + "\nSshEnabled=" + (ssh ? "true\n" : "false\n"), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther)) return false;
    if (remote) {
        if (!writeConfig("/etc/fidan/remote-enabled", "enabled\n", QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther)
            || !run("/usr/bin/systemctl", {"enable", "fidan-gateway.service", "fidan-remote-broker.service"})) return false;
    } else if (QFile::exists(path("/etc/fidan/remote-enabled")) && !QFile::remove(path("/etc/fidan/remote-enabled"))) return false;
    m_state["remoteEnabled"] = remote;
    m_state["sshEnabled"] = ssh; m_state["diagnosticsEnabled"] = diagnostics; m_state["privacyDone"] = true;
    emit Progress("privacy"); return saveState();
}
bool FirstBootService::StartUpdate(const QString &action)
{
    if (!callerIsSetupUser() || !m_state.value("accountReady").toBool() || updateRunning()
        || !QStringList{"check", "download", "install"}.contains(action)) return false;
    m_updateError.clear(); m_updateAction = action;
    // A dedicated transient system service applies packages outside this daemon's
    // read-only filesystem sandbox. Only the fixed updater and these three actions can run.
    m_update.start("/usr/bin/systemd-run", {"--quiet", "--wait", "--pipe", "--collect", "--unit=fidan-firstboot-update", "/usr/bin/fidan-update", action});
    m_updateTimeout.start(); return true;
}
bool FirstBootService::SetDesktopPreferences(const QVariantMap &preferences)
{
    if (!callerIsSetupUser() || !m_state.value("accountReady").toBool()) return false;
    if (!FirstBootValidation::desktopPreferences(preferences)) {
        fail("org.fidan.Error.InvalidInput",tr("Choose supported desktop preferences.")); return false;
    }
    m_state["desktopPreferences"]=preferences; m_state["desktopPreferencesReady"]=true;
    return saveState();
}
bool FirstBootService::SkipUpdates()
{
    if (!callerIsSetupUser() || !m_state.value("accountReady").toBool() || updateRunning()) return false;
    m_state["updateDone"] = true; m_state["updatesSkipped"] = true; return saveState();
}
bool FirstBootService::Complete()
{
    if (!callerIsSetupUser() || updateRunning()) return false;
    for (const auto &stage : {"localeReady", "accountReady", "networkDone", "privacyDone", "updateDone"})
        if (!m_state.value(stage).toBool()) { fail("org.fidan.Error.Incomplete", tr("Finish or explicitly skip each setup step first.")); return false; }
    if (!ensureAccountHome(m_state.value("username").toString())) {
        fail("org.fidan.Error.HomeUnavailable", tr("The account home is not ready. Setup was saved; repair it before finishing.")); return false;
    }
    if (!run("/usr/bin/getent", {"passwd", m_state.value("username").toString()})
        || !run("/usr/bin/test", {"-x", path("/usr/bin/fidan-session")})
        || !run("/usr/bin/systemctl", {"enable", "greetd.service"})
        || !run("/usr/bin/systemctl", {"is-enabled", "--quiet", "greetd.service"})) {
        fail("org.fidan.Error.LoginUnavailable", tr("The login service is not ready. Setup was saved; retry after fixing the service.")); return false;
    }
    if (m_state.value("desktopPreferencesReady").toBool()) {
        const auto preferences=m_state.value("desktopPreferences").toMap();
        if (!FirstBootValidation::desktopPreferences(preferences)
            || !run("/usr/bin/systemd-run",{"--quiet","--wait","--pipe","--collect","--unit=fidan-setup-profile",
                       "--uid="+m_state.value("username").toString(),"--property=NoNewPrivileges=yes","--","/usr/bin/fidan-setup-preferences"},
                    QJsonDocument::fromVariant(preferences).toJson(QJsonDocument::Compact))) {
            fail("org.fidan.Error.PreferencesFailed",tr("The account preferences could not be saved. Retry before finishing.")); return false;
        }
    }
    // Both sessions have opposite marker conditions. Keep setup enabled for
    // recovery until this atomic commit; a failed login enable never locks it out.
    if (!writeConfig("/var/lib/fidan/firstboot-complete", "complete\n", QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther)) return false;
    emit Progress("complete");
    run("/usr/bin/fidan-remote", {"cleanup-setup"});
    run("/usr/bin/systemd-run", {"--unit=fidan-firstboot-reboot", "--on-active=3s", "/usr/bin/systemctl", "reboot"});
    return true;
}
bool FirstBootService::IsComplete() const { return QFileInfo::exists(path("/var/lib/fidan/firstboot-complete")); }
