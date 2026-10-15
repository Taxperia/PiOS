// SPDX-License-Identifier: GPL-3.0-or-later
#include "ShellController.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QLocale>
#include <QProcess>
#include "../common/DesktopLaunch.h"
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QSysInfo>
#include <QMap>
#include <QUrl>
#include <QMimeDatabase>
#include <QSet>
#include <QColor>
#include <QTime>

#include <algorithm>

namespace {
bool supportedDefault(const QString &mime) {
    return QStringList{QStringLiteral("x-scheme-handler/http"), QStringLiteral("text/plain"),
        QStringLiteral("image/png"), QStringLiteral("video/mp4"), QStringLiteral("audio/mpeg")}.contains(mime);
}
QString settingsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/fidan/shell.conf");
}

}

QVariantList ShellController::applicationChoices(const QString &mime) const
{
    if (!supportedDefault(mime)) return {};
    QVariantList result;
    QSet<QString> seen;
    QStringList roots = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    roots.prepend(QDir::homePath() + QStringLiteral("/.local/share/flatpak/exports/share/applications"));
    roots.append(QStringLiteral("/var/lib/flatpak/exports/share/applications"));
    for (const auto &root : roots) for (const auto &file : QDir(root).entryInfoList({QStringLiteral("*.desktop")}, QDir::Files, QDir::Name)) {
        const QString id = file.fileName();
        if (seen.contains(id)) continue;
        seen.insert(id);
        // Desktop entries use semicolon lists; QSettings treats unquoted
        // semicolons as INI comments and would lose all but the first MIME.
        QFile entry(file.absoluteFilePath());
        if (!entry.open(QIODevice::ReadOnly)) continue;
        QHash<QString, QString> desktop;
        bool group = false;
        for (const auto &bytes : entry.readAll().split('\n')) {
            const QString line = QString::fromUtf8(bytes).trimmed();
            if (line.startsWith(QLatin1Char('['))) { group = line == QStringLiteral("[Desktop Entry]"); continue; }
            const int separator = line.indexOf(QLatin1Char('='));
            if (group && separator > 0) desktop.insert(line.left(separator), line.mid(separator + 1));
        }
        if (desktop.value(QStringLiteral("Hidden")) == QStringLiteral("true") || desktop.value(QStringLiteral("NoDisplay")) == QStringLiteral("true")) continue;
        const QStringList types = desktop.value(QStringLiteral("MimeType")).split(QLatin1Char(';'));
        bool matches = types.contains(mime);
        for (const auto &type : types) if (!type.isEmpty() && QMimeDatabase().mimeTypeForName(mime).inherits(type)) matches = true;
        if (!matches) continue;
        const QString name = desktop.value(QStringLiteral("Name"));
        if (!name.isEmpty()) result.append(QVariantMap{{QStringLiteral("id"), id}, {QStringLiteral("name"), name}});
    }
    return result;
}

QString ShellController::defaultApplication(const QString &mime) const
{
    if (!supportedDefault(mime)) return {};
    QProcess query;
    query.start(QStringLiteral("xdg-mime"), {QStringLiteral("query"), QStringLiteral("default"), mime});
    if (!query.waitForFinished(1000) || query.exitCode() != 0) return {};
    return QString::fromUtf8(query.readAllStandardOutput()).trimmed();
}

bool ShellController::setDefaultApplication(const QString &mime, const QString &desktopId)
{
    bool valid = false;
    for (const auto &choice : applicationChoices(mime)) if (choice.toMap().value(QStringLiteral("id")).toString() == desktopId) valid = true;
    if (!valid) { emit actionFailed(tr("Choose an installed application that supports this file type.")); return false; }
    QStringList types{mime};
    if (mime == QStringLiteral("x-scheme-handler/http")) types.append(QStringLiteral("x-scheme-handler/https"));
    QProcess update;
    for (const auto &type : types) {
        update.start(QStringLiteral("xdg-mime"), {QStringLiteral("default"), desktopId, type});
        if (!update.waitForFinished(1500) || update.exitCode() != 0) {
            emit actionFailed(tr("The default application could not be changed.")); return false;
        }
    }
    return true;
}

ShellController::ShellController(bool previewMode, bool mockMode, QObject *parent)
    : QObject(parent)
    , m_previewMode(previewMode)
    , m_mockMode(mockMode)
    , m_pinnedApps({QStringLiteral("pcmanfm-qt.desktop"), QStringLiteral("qterminal.desktop"),
                    QStringLiteral("chromium.desktop"), QStringLiteral("featherpad.desktop")})
    , m_clockTimer(new QTimer(this))
    , m_connectivityTimer(new QTimer(this))
    , m_windowTimer(new QTimer(this))
    , m_windowQuery(new QProcess(this))
{
    QSettings settings(settingsPath(), QSettings::IniFormat);
    m_powerProfilesAvailable = !QStandardPaths::findExecutable(QStringLiteral("powerprofilesctl")).isEmpty();
    m_lockAvailable = m_previewMode
        || !QStandardPaths::findExecutable(QStringLiteral("fidan-lock")).isEmpty()
        || !QStandardPaths::findExecutable(QStringLiteral("gtklock")).isEmpty();
    m_wifiAvailable = m_mockMode;
    m_wifiEnabled = m_mockMode;
    m_bluetoothAvailable = m_mockMode;
    m_audioAvailable = m_mockMode;
    m_powerActionsAvailable = m_mockMode;
    if (!m_mockMode) {
        QDBusInterface login(QStringLiteral("org.freedesktop.login1"),
                             QStringLiteral("/org/freedesktop/login1"),
                             QStringLiteral("org.freedesktop.login1.Manager"),
                             QDBusConnection::systemBus());
        if (login.isValid()) {
            const QDBusReply<QString> canReboot = login.call(QStringLiteral("CanReboot"));
            const QDBusReply<QString> canPowerOff = login.call(QStringLiteral("CanPowerOff"));
            const auto allowed = [](const QDBusReply<QString> &reply) {
                return reply.isValid()
                    && (reply.value() == QStringLiteral("yes")
                        || reply.value() == QStringLiteral("challenge"));
            };
            m_powerActionsAvailable = allowed(canReboot) && allowed(canPowerOff);
        }
    }
    m_darkTheme = settings.value(QStringLiteral("appearance/darkTheme"), true).toBool();
    m_reducedMotion = settings.value(QStringLiteral("accessibility/reducedMotion"), false).toBool();
    m_interfaceScale = std::clamp(settings.value(QStringLiteral("appearance/interfaceScale"), 1.25).toDouble(),
                                  1.0, 1.5);
    m_volume = settings.value(QStringLiteral("audio/volume"), 62).toInt();
    m_brightness = settings.value(QStringLiteral("display/brightness"), 58).toInt();
    m_brightnessAvailable = m_mockMode;
    m_firstBootVisible = m_previewMode
        && !settings.value(QStringLiteral("preview/firstBootSeen"), false).toBool()
        && !qEnvironmentVariableIsSet("FIDAN_SKIP_FIRSTBOOT");
    m_quickSettingsVisible = m_previewMode && qEnvironmentVariableIsSet("FIDAN_PREVIEW_QUICK_SETTINGS");
    m_settingsVisible = m_previewMode && qEnvironmentVariableIsSet("FIDAN_PREVIEW_SETTINGS");
    m_softwareCenterVisible = m_previewMode && qEnvironmentVariableIsSet("FIDAN_PREVIEW_SOFTWARE");
    const QStringList pins = settings.value(QStringLiteral("dock/pinnedApps")).toStringList();
    if (!pins.isEmpty()) {
        m_pinnedApps = pins;
    }

    reloadPreferences();
    m_clockTimer->setInterval(1000);
    connect(m_clockTimer, &QTimer::timeout, this, &ShellController::clockChanged);
    m_clockTimer->start();

    m_connectivityTimer->setInterval(5000);
    connect(m_connectivityTimer, &QTimer::timeout, this, &ShellController::refreshConnectivity);
    m_connectivityTimer->start();
    refreshConnectivity();
    refreshAudio();
    refreshBrightness();

    connect(m_windowQuery, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
            return;
        }
        QStringList apps;
        const QString output = QString::fromUtf8(m_windowQuery->readAllStandardOutput());
        for (const QString &line : output.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
            const QString appId = line.section(QLatin1Char(':'), 0, 0).trimmed();
            if (!appId.isEmpty() && appId != QStringLiteral("fidan-shell")) {
                apps.append(appId);
            }
        }
        apps.removeDuplicates();
        if (m_runningApps != apps) {
            m_runningApps = apps;
            emit runningAppsChanged();
        }
    });
    m_windowTimer->setInterval(2500);
    connect(m_windowTimer, &QTimer::timeout, this, &ShellController::refreshRunningApps);
    connect(m_windowTimer, &QTimer::timeout, this, &ShellController::reloadPreferences);
    m_windowTimer->start();
    refreshRunningApps();

}

void ShellController::setDarkTheme(bool darkTheme)
{
    setThemeMode(darkTheme ? QStringLiteral("dark") : QStringLiteral("light"));
}

void ShellController::setThemeMode(const QString &value) {
    if (!QStringList{"light", "dark", "auto"}.contains(value)) return;
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue("appearance/themeMode", value); settings.sync(); reloadPreferences();
}
void ShellController::setAccentColor(const QString &value) {
    const QColor color(value);
    if (!color.isValid() || color.alpha() != 255) return;
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue("appearance/accentColor", color.name()); settings.sync(); reloadPreferences();
}
void ShellController::setDockPosition(const QString &value) {
    if (!QStringList{"left", "bottom", "right"}.contains(value)) return;
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue("dock/position", value); settings.sync(); reloadPreferences();
}
void ShellController::setDockIconSize(int value) {
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue("dock/iconSize", std::clamp(value, 20, 48)); settings.sync(); reloadPreferences();
}
void ShellController::setLockWallpaperUrl(const QString &value) {
    if (!value.isEmpty() && (!QUrl(value).isLocalFile() || !QFileInfo(QUrl(value).toLocalFile()).isFile() || !QFileInfo(QUrl(value).toLocalFile()).isReadable())) {
        emit actionFailed(tr("The lock screen wallpaper cannot be read.")); return;
    }
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue("appearance/lockWallpaper", value); settings.sync(); reloadPreferences();
}

void ShellController::setReducedMotion(bool reducedMotion)
{
    if (m_reducedMotion == reducedMotion) {
        return;
    }
    m_reducedMotion = reducedMotion;
    saveSettings();
    emit reducedMotionChanged();
}

void ShellController::setInterfaceScale(double scale)
{
    const double bounded = std::clamp(scale, 1.0, 1.5);
    if (qFuzzyCompare(m_interfaceScale, bounded)) {
        return;
    }
    m_interfaceScale = bounded;
    saveSettings();
    emit interfaceScaleChanged();
}

void ShellController::setQuickSettingsVisible(bool visible)
{
    if (m_quickSettingsVisible == visible) {
        return;
    }
    m_quickSettingsVisible = visible;
    if (visible) {
        setLauncherVisible(false);
        if (m_statusCardVisible) {
            m_statusCardVisible = false;
            emit statusCardVisibleChanged();
        }
    }
    emit quickSettingsVisibleChanged();
}

void ShellController::setLauncherVisible(bool visible)
{
    if (m_launcherVisible == visible) {
        return;
    }
    m_launcherVisible = visible;
    if (visible) {
        setQuickSettingsVisible(false);
        if (m_statusCardVisible) {
            m_statusCardVisible = false;
            emit statusCardVisibleChanged();
        }
    }
    emit launcherVisibleChanged();
}

void ShellController::setVolume(int volume)
{
    if (!m_audioAvailable) {
        emit actionFailed(tr("Audio output is unavailable."));
        return;
    }
    const int bounded = std::clamp(volume, 0, 100);
    if (m_volume == bounded) {
        return;
    }
    m_volume = bounded;
    if (m_volumeMuted && m_volume > 0) {
        m_volumeMuted = false;
        emit volumeMutedChanged();
        if (!m_mockMode) {
            startDesktopProcess(QStringLiteral("pactl"),
                                    {QStringLiteral("set-sink-mute"),
                                     QStringLiteral("@DEFAULT_SINK@"), QStringLiteral("0")});
        }
    }
    saveSettings();
    emit volumeChanged();
    if (!m_mockMode) {
        startDesktopProcess(QStringLiteral("pactl"),
                                {QStringLiteral("set-sink-volume"), QStringLiteral("@DEFAULT_SINK@"),
                                 QString::number(m_volume) + QLatin1Char('%')});
    }
}

void ShellController::toggleVolumeMute()
{
    if (!m_audioAvailable) {
        emit actionFailed(tr("Audio output is unavailable."));
        return;
    }
    m_volumeMuted = !m_volumeMuted;
    emit volumeMutedChanged();
    if (!m_mockMode) {
        startDesktopProcess(QStringLiteral("pactl"),
                                {QStringLiteral("set-sink-mute"),
                                 QStringLiteral("@DEFAULT_SINK@"),
                                 m_volumeMuted ? QStringLiteral("1") : QStringLiteral("0")});
    }
}

void ShellController::setBrightness(int brightness)
{
    const int bounded = std::clamp(brightness, 1, 100);
    if (!m_brightnessAvailable || m_brightness == bounded) {
        return;
    }
    m_brightness = bounded;
    saveSettings();
    emit brightnessChanged();
    if (!m_mockMode) {
        startDesktopProcess(QStringLiteral("brightnessctl"),
                                {QStringLiteral("set"), QString::number(m_brightness) + QLatin1Char('%')});
    }
}

QString ShellController::clockText() const
{
    return QLocale().toString(QDateTime::currentDateTime().time(), QStringLiteral("HH:mm"));
}

QString ShellController::dateText() const
{
    return QLocale().toString(QDate::currentDate(), QStringLiteral("ddd d MMM"));
}

void ShellController::toggleQuickSettings()
{
    setQuickSettingsVisible(!m_quickSettingsVisible);
}

void ShellController::toggleLauncher()
{
    setLauncherVisible(!m_launcherVisible);
}

void ShellController::openFileManager()
{
    setLauncherVisible(false);
    if (!startDesktopProcess(QStringLiteral("fidan-files"))
        && !startDesktopProcess(QStringLiteral("pcmanfm-qt"))) {
        emit actionFailed(tr("File Manager is unavailable."));
    }
}

void ShellController::openSettings()
{
    if (m_previewMode && m_mockMode) {
        if (!m_settingsVisible) {
            m_settingsVisible = true;
            emit settingsVisibleChanged();
        }
        setLauncherVisible(false);
        setQuickSettingsVisible(false);
        return;
    }
    const QStringList candidates = {QStringLiteral("fidan-settings"),
                                    QStringLiteral("rpd-preferences"),
                                    QStringLiteral("lxqt-config")};
    for (const QString &candidate : candidates) {
        if (startDesktopProcess(candidate)) {
            return;
        }
    }
    emit actionFailed(tr("System settings are unavailable."));
}

void ShellController::closeSettings()
{
    if (m_settingsVisible) {
        m_settingsVisible = false;
        emit settingsVisibleChanged();
    }
}

void ShellController::openSoftwareCenter()
{
    if (m_previewMode && m_mockMode) {
        if (!m_softwareCenterVisible) {
            m_softwareCenterVisible = true;
            emit softwareCenterVisibleChanged();
        }
        setLauncherVisible(false);
        setQuickSettingsVisible(false);
        return;
    }

    const QStringList candidates = {QStringLiteral("fidan-store"),
                                    QStringLiteral("gnome-software"),
                                    QStringLiteral("plasma-discover")};
    for (const QString &candidate : candidates) {
        if (startDesktopProcess(candidate)) {
            return;
        }
    }
    emit actionFailed(tr("Software Center is unavailable."));
}

void ShellController::closeSoftwareCenter()
{
    if (m_softwareCenterVisible) {
        m_softwareCenterVisible = false;
        emit softwareCenterVisibleChanged();
    }
}

void ShellController::openTerminal()
{
    if (!startDesktopProcess(QStringLiteral("qterminal"))) {
        emit actionFailed(tr("Terminal is unavailable."));
    }
}

void ShellController::openTaskManager()
{
    if (!startDesktopProcess(QStringLiteral("fidan-task-manager"))) {
        emit actionFailed(tr("Task Manager is unavailable."));
    }
}

QString ShellController::desktopPath() const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (path.isEmpty()) {
        path = QDir::home().filePath(QStringLiteral("Desktop"));
    }
    return path;
}

QString ShellController::uniqueDesktopName(const QString &baseName, const QString &suffix) const
{
    const QDir desktop(desktopPath());
    QString name = baseName + suffix;
    for (int number = 2; desktop.exists(name); ++number) {
        name = QStringLiteral("%1 %2%3").arg(baseName).arg(number).arg(suffix);
    }
    return name;
}

QString ShellController::createDesktopFolder()
{
    const QString name = uniqueDesktopName(tr("New Folder"), QString());
    if (m_mockMode) {
        return name;
    }

    QDir desktop(desktopPath());
    if ((!desktop.exists() && !QDir().mkpath(desktop.absolutePath())) || !desktop.mkdir(name)) {
        emit actionFailed(tr("The desktop folder could not be created."));
        return {};
    }
    return name;
}

QString ShellController::createDesktopFile()
{
    const QString name = uniqueDesktopName(tr("New File"), QStringLiteral(".txt"));
    if (m_mockMode) {
        return name;
    }

    QDir desktop(desktopPath());
    if (!desktop.exists() && !QDir().mkpath(desktop.absolutePath())) {
        emit actionFailed(tr("The desktop directory is unavailable."));
        return {};
    }
    QFile file(desktop.filePath(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        emit actionFailed(tr("The desktop file could not be created."));
        return {};
    }
    return name;
}

bool ShellController::openDesktopTerminal()
{
    if (QStandardPaths::findExecutable(QStringLiteral("qterminal")).isEmpty()) {
        emit actionFailed(tr("Terminal is unavailable."));
        return false;
    }
    return startDesktopProcess(QStringLiteral("qterminal"),
                                   {QStringLiteral("--workdir"), desktopPath()});
}

void ShellController::toggleStatusCard()
{
    m_statusCardVisible = !m_statusCardVisible;
    if (m_statusCardVisible) {
        setQuickSettingsVisible(false);
    }
    emit statusCardVisibleChanged();
}

void ShellController::showFirstBoot()
{
    if (!m_firstBootVisible) {
        m_firstBootVisible = true;
        setLauncherVisible(false);
        emit firstBootVisibleChanged();
    }
}

void ShellController::completeFirstBootPreview()
{
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("preview/firstBootSeen"), true);
    if (m_firstBootVisible) {
        m_firstBootVisible = false;
        emit firstBootVisibleChanged();
        lockSession();
    }
}

void ShellController::unlockPreview()
{
    if (m_lockScreenVisible) {
        m_lockScreenVisible = false;
        emit lockScreenVisibleChanged();
    }
}

void ShellController::toggleWifi()
{
    if (m_mockMode) {
        m_wifiEnabled = !m_wifiEnabled;
        emit connectivityChanged();
        return;
    }
    if (!m_wifiAvailable) {
        emit actionFailed(tr("Wi-Fi hardware is unavailable."));
        return;
    }
    if (setDbusProperty(QStringLiteral("org.freedesktop.NetworkManager"),
                        QStringLiteral("/org/freedesktop/NetworkManager"),
                        QStringLiteral("org.freedesktop.NetworkManager"),
                        QStringLiteral("WirelessEnabled"), !m_wifiEnabled)) {
        m_wifiEnabled = !m_wifiEnabled;
        emit connectivityChanged();
    }
}

void ShellController::toggleBluetooth()
{
    if (m_mockMode) {
        m_bluetoothEnabled = !m_bluetoothEnabled;
        emit connectivityChanged();
        return;
    }
    if (!m_bluetoothAvailable) {
        emit actionFailed(tr("Bluetooth adapter is unavailable."));
        return;
    }

    // Raspberry Pi OS exposes its built-in controller as hci0. BlueZ returns a
    // normal D-Bus error when the controller is absent.
    const QString adapterPath = QStringLiteral("/org/bluez/hci0");
    if (!setDbusProperty(QStringLiteral("org.bluez"), adapterPath,
                            QStringLiteral("org.bluez.Adapter1"), QStringLiteral("Powered"),
                            !m_bluetoothEnabled)) {
        emit actionFailed(tr("Bluetooth adapter is unavailable."));
        return;
    }
    m_bluetoothEnabled = !m_bluetoothEnabled;
    emit connectivityChanged();
}

void ShellController::activateWorkspace(int workspace)
{
    if (workspace < 1 || workspace > 3 || workspace == m_activeWorkspace) {
        return;
    }
    m_activeWorkspace = workspace;
    emit activeWorkspaceChanged();
    if (!m_mockMode) {
        startDesktopProcess(QStringLiteral("fidan-workspace"), {QString::number(workspace)});
    }
}

void ShellController::lockSession()
{
    if (m_previewMode) {
        if (!m_lockScreenVisible) {
            m_lockScreenVisible = true;
            setQuickSettingsVisible(false);
            setLauncherVisible(false);
            emit lockScreenVisibleChanged();
        }
        return;
    }
    if (!startDesktopProcess(QStringLiteral("fidan-lock"))) {
        startDesktopProcess(QStringLiteral("loginctl"), {QStringLiteral("lock-session")});
    }
}

void ShellController::requestPowerAction(const QString &action)
{
    if (m_mockMode) {
        return;
    }
    const QString method = action == QStringLiteral("restart")
        ? QStringLiteral("Reboot")
        : action == QStringLiteral("shutdown") ? QStringLiteral("PowerOff") : QString();
    if (method.isEmpty()) {
        emit actionFailed(tr("Unknown power action."));
        return;
    }

    QDBusInterface login(QStringLiteral("org.freedesktop.login1"),
                         QStringLiteral("/org/freedesktop/login1"),
                         QStringLiteral("org.freedesktop.login1.Manager"),
                         QDBusConnection::systemBus());
    const QDBusMessage reply = login.call(method, true);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        emit actionFailed(reply.errorMessage());
    }
}

bool ShellController::launchPinned(const QString &desktopId)
{
    QStringList roots = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    roots.append(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                 + QStringLiteral("/flatpak/exports/share/applications"));
    roots.append(QStringLiteral("/var/lib/flatpak/exports/share/applications"));
    QStringList ids { desktopId };
    if (desktopId == QStringLiteral("vlc.desktop"))
        ids.append(QStringLiteral("org.videolan.VLC.desktop"));
    for (const QString &root : roots) {
        for (const QString &id : ids) {
            const QString path = QDir(root).filePath(id);
            if (QFileInfo::exists(path)) {
                if (startDesktopProcess(QStringLiteral("gtk-launch"), {id.chopped(8)})) return true;
                emit actionFailed(tr("This application could not be started."));
                return false;
            }
        }
    }
    emit actionFailed(tr("This application is not installed. Open Fidan Store to install it."));
    return false;
}

bool ShellController::focusRunningApp(const QString &appId)
{
    if (appId.isEmpty() || m_mockMode) {
        return m_mockMode;
    }
    const bool started = startDesktopProcess(QStringLiteral("wlrctl"),
        {QStringLiteral("toplevel"), QStringLiteral("focus"), QStringLiteral("app_id:") + appId});
    if (!started) emit actionFailed(tr("The window could not be restored."));
    return started;
}

void ShellController::refreshRunningApps()
{
    if (m_mockMode) {
        const QStringList mockApps = {QStringLiteral("qterminal"), QStringLiteral("chromium")};
        if (m_runningApps != mockApps) {
            m_runningApps = mockApps;
            emit runningAppsChanged();
        }
        return;
    }
    if (m_windowQuery->state() == QProcess::NotRunning
        && !QStandardPaths::findExecutable(QStringLiteral("wlrctl")).isEmpty()) {
        m_windowQuery->start(QStringLiteral("wlrctl"),
                             {QStringLiteral("toplevel"), QStringLiteral("list")});
    }
}

void ShellController::cyclePowerProfile()
{
    if (!m_powerProfilesAvailable) {
        return;
    }
    if (m_powerProfile == QStringLiteral("power-saver")) {
        m_powerProfile = QStringLiteral("balanced");
    } else if (m_powerProfile == QStringLiteral("balanced")) {
        m_powerProfile = QStringLiteral("performance");
    } else {
        m_powerProfile = QStringLiteral("power-saver");
    }
    if (!m_mockMode) {
        startDesktopProcess(QStringLiteral("powerprofilesctl"), {QStringLiteral("set"), m_powerProfile});
    }
    emit powerProfileChanged();
}

void ShellController::saveSettings()
{
    QDir().mkpath(QFileInfo(settingsPath()).absolutePath());
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("appearance/darkTheme"), m_darkTheme);
    settings.setValue(QStringLiteral("accessibility/reducedMotion"), m_reducedMotion);
    settings.setValue(QStringLiteral("appearance/interfaceScale"), m_interfaceScale);
    settings.setValue(QStringLiteral("audio/volume"), m_volume);
    settings.setValue(QStringLiteral("display/brightness"), m_brightness);
    settings.setValue(QStringLiteral("dock/pinnedApps"), m_pinnedApps);
    settings.setValue(QStringLiteral("appearance/wallpaper"), m_wallpaperUrl);
    settings.setValue(QStringLiteral("desktop/showIcons"), m_desktopIconsVisible);
    settings.setValue(QStringLiteral("privacy/notifications"), m_notificationsEnabled);
    settings.setValue(QStringLiteral("input/keyboardLayout"), m_keyboardLayout);
}

void ShellController::refreshConnectivity()
{
    if (m_mockMode) {
        return;
    }
    QDBusInterface manager(QStringLiteral("org.freedesktop.NetworkManager"),
                           QStringLiteral("/org/freedesktop/NetworkManager"),
                           QStringLiteral("org.freedesktop.NetworkManager"),
                           QDBusConnection::systemBus());
    bool wifiAvailable = false;
    if (manager.isValid()) {
        const QDBusReply<QList<QDBusObjectPath>> devices = manager.call(QStringLiteral("GetDevices"));
        if (devices.isValid()) {
            for (const QDBusObjectPath &path : devices.value()) {
                QDBusInterface device(QStringLiteral("org.freedesktop.NetworkManager"), path.path(),
                                      QStringLiteral("org.freedesktop.NetworkManager.Device"),
                                      QDBusConnection::systemBus());
                // NetworkManager DeviceType 2 is a Wi-Fi adapter. The global
                // radio flag alone is also true on hosts with only Ethernet.
                if (device.isValid() && device.property("DeviceType").toUInt() == 2) {
                    wifiAvailable = true;
                    break;
                }
            }
        }
    }
    if (m_wifiAvailable != wifiAvailable) {
        m_wifiAvailable = wifiAvailable;
        emit connectivityChanged();
    }
    if (wifiAvailable) {
        const bool enabled = manager.property("WirelessEnabled").toBool();
        if (m_wifiEnabled != enabled) {
            m_wifiEnabled = enabled;
            emit connectivityChanged();
        }
    } else if (m_wifiEnabled) {
        m_wifiEnabled = false;
        emit connectivityChanged();
    }

    QDBusInterface bluezProperties(QStringLiteral("org.bluez"),
                                   QStringLiteral("/org/bluez/hci0"),
                                   QStringLiteral("org.freedesktop.DBus.Properties"),
                                   QDBusConnection::systemBus());
    bool bluetoothAvailable = false;
    if (bluezProperties.isValid()) {
        const QDBusReply<QDBusVariant> powered = bluezProperties.call(
            QStringLiteral("Get"), QStringLiteral("org.bluez.Adapter1"),
            QStringLiteral("Powered"));
        if (powered.isValid()) {
            bluetoothAvailable = true;
            const bool enabled = powered.value().variant().toBool();
            if (m_bluetoothEnabled != enabled) {
                m_bluetoothEnabled = enabled;
                emit connectivityChanged();
            }
        }
    }
    if (m_bluetoothAvailable != bluetoothAvailable) {
        m_bluetoothAvailable = bluetoothAvailable;
        emit connectivityChanged();
    }
    if (!bluetoothAvailable && m_bluetoothEnabled) {
        m_bluetoothEnabled = false;
        emit connectivityChanged();
    }
    refreshAudio();
    refreshBrightness();
}

void ShellController::refreshAudio()
{
    if (m_mockMode || QStandardPaths::findExecutable(QStringLiteral("pactl")).isEmpty()) {
        return;
    }

    QProcess infoProcess;
    infoProcess.start(QStringLiteral("pactl"), {QStringLiteral("info")});
    const bool available = infoProcess.waitForFinished(700) && infoProcess.exitCode() == 0;
    if (m_audioAvailable != available) {
        m_audioAvailable = available;
        emit audioAvailabilityChanged();
    }
    if (!available) {
        return;
    }

    QProcess volumeProcess;
    volumeProcess.start(QStringLiteral("pactl"),
                        {QStringLiteral("get-sink-volume"), QStringLiteral("@DEFAULT_SINK@")});
    if (volumeProcess.waitForFinished(700)) {
        const QString output = QString::fromUtf8(volumeProcess.readAllStandardOutput());
        const QRegularExpressionMatch match = QRegularExpression(QStringLiteral("(\\d+)%")).match(output);
        if (match.hasMatch()) {
            const int currentVolume = std::clamp(match.captured(1).toInt(), 0, 100);
            if (m_volume != currentVolume) {
                m_volume = currentVolume;
                emit volumeChanged();
            }
        }
    }

    QProcess muteProcess;
    muteProcess.start(QStringLiteral("pactl"),
                      {QStringLiteral("get-sink-mute"), QStringLiteral("@DEFAULT_SINK@")});
    if (muteProcess.waitForFinished(700)) {
        const bool muted = QString::fromUtf8(muteProcess.readAllStandardOutput())
                               .contains(QStringLiteral("yes"), Qt::CaseInsensitive);
        if (m_volumeMuted != muted) {
            m_volumeMuted = muted;
            emit volumeMutedChanged();
        }
    }
}

void ShellController::refreshBrightness()
{
    if (m_mockMode) {
        return;
    }
    if (QStandardPaths::findExecutable(QStringLiteral("brightnessctl")).isEmpty()) {
        if (m_brightnessAvailable) {
            m_brightnessAvailable = false;
            emit brightnessAvailabilityChanged();
        }
        return;
    }
    QProcess process;
    process.start(QStringLiteral("brightnessctl"), {QStringLiteral("-m")});
    if (!process.waitForFinished(700) || process.exitCode() != 0) {
        if (m_brightnessAvailable) {
            m_brightnessAvailable = false;
            emit brightnessAvailabilityChanged();
        }
        return;
    }
    const QStringList fields = QString::fromUtf8(process.readAllStandardOutput())
                                   .trimmed().split(QLatin1Char(','));
    if (fields.size() < 5) {
        return;
    }
    if (!m_brightnessAvailable) {
        m_brightnessAvailable = true;
        emit brightnessAvailabilityChanged();
    }
    QString percentage = fields.last().trimmed();
    percentage.remove(QLatin1Char('%'));
    const int currentBrightness = std::clamp(percentage.toInt(), 1, 100);
    if (m_brightness != currentBrightness) {
        m_brightness = currentBrightness;
        emit brightnessChanged();
    }
}

bool ShellController::setDbusProperty(const QString &service, const QString &path,
                                      const QString &interface, const QString &property,
                                      const QVariant &value)
{
    QDBusInterface properties(service, path, QStringLiteral("org.freedesktop.DBus.Properties"),
                              QDBusConnection::systemBus());
    const QDBusMessage reply = properties.call(QStringLiteral("Set"), interface, property,
                                                QVariant::fromValue(QDBusVariant(value)));
    if (reply.type() == QDBusMessage::ErrorMessage) {
        emit actionFailed(reply.errorMessage());
        return false;
    }
    return true;
}


void ShellController::reloadPreferences()
{
    QSettings settings(settingsPath(), QSettings::IniFormat);
    QString mode = settings.value("appearance/themeMode", settings.value("appearance/darkTheme", true).toBool() ? "dark" : "light").toString();
    if (!QStringList{"light", "dark", "auto"}.contains(mode)) mode = "dark";
    const bool dark = mode == "dark" || (mode == "auto" && (QTime::currentTime().hour() < 8 || QTime::currentTime().hour() >= 19));
    const QColor storedAccent(settings.value("appearance/accentColor", "#63de8a").toString());
    const QString accent = storedAccent.isValid() && storedAccent.alpha() == 255 ? storedAccent.name() : "#63de8a";
    QString dock = settings.value("dock/position", "left").toString();
    if (!QStringList{"left", "bottom", "right"}.contains(dock)) dock = "left";
    const int iconSize = std::clamp(settings.value("dock/iconSize", 24).toInt(), 20, 48);
    const QString lockWallpaper = settings.value("appearance/lockWallpaper").toString();
    if (m_themeMode != mode || m_accentColor != accent || m_dockPosition != dock || m_dockIconSize != iconSize || m_lockWallpaperUrl != lockWallpaper) {
        m_themeMode = mode; m_accentColor = accent; m_dockPosition = dock; m_dockIconSize = iconSize; m_lockWallpaperUrl = lockWallpaper;
        emit preferencesChanged();
    }
    if (settings.value("appearance/darkTheme", true).toBool() != dark) settings.setValue("appearance/darkTheme", dark);
    const bool reduced = settings.value(QStringLiteral("accessibility/reducedMotion"), false).toBool();
    const double scale = std::clamp(settings.value(QStringLiteral("appearance/interfaceScale"), 1.25).toDouble(), 1.0, 1.5);
    if (m_darkTheme != dark) { m_darkTheme = dark; emit darkThemeChanged(); }
    if (m_reducedMotion != reduced) { m_reducedMotion = reduced; emit reducedMotionChanged(); }
    if (!qFuzzyCompare(m_interfaceScale, scale)) { m_interfaceScale = scale; emit interfaceScaleChanged(); }
    const QString wallpaper = settings.value(QStringLiteral("appearance/wallpaper")).toString();
    const bool icons = settings.value(QStringLiteral("desktop/showIcons"), true).toBool();
    const bool notifications = settings.value(QStringLiteral("privacy/notifications"), true).toBool();
    const QString keyboard = settings.value(QStringLiteral("input/keyboardLayout"), QStringLiteral("us")).toString();
    if (m_wallpaperUrl != wallpaper || m_desktopIconsVisible != icons || m_notificationsEnabled != notifications || m_keyboardLayout != keyboard) {
        m_wallpaperUrl = wallpaper; m_desktopIconsVisible = icons;
        m_notificationsEnabled = notifications; m_keyboardLayout = keyboard;
        emit preferencesChanged();
    }
}

void ShellController::setWallpaperUrl(const QString &value)
{
    if (value == m_wallpaperUrl) return;
    if (!value.isEmpty() && !QFileInfo(QUrl(value).toLocalFile()).isReadable()) {
        emit actionFailed(tr("The wallpaper file cannot be read.")); return;
    }
    m_wallpaperUrl = value; saveSettings(); emit preferencesChanged();
}
void ShellController::setDesktopIconsVisible(bool value) { if (m_desktopIconsVisible == value) return; m_desktopIconsVisible = value; saveSettings(); emit preferencesChanged(); }
void ShellController::setNotificationsEnabled(bool value) { if (m_notificationsEnabled == value) return; m_notificationsEnabled = value; saveSettings(); emit preferencesChanged(); }
void ShellController::setKeyboardLayout(const QString &value) {
    if (!QStringList{QStringLiteral("us"), QStringLiteral("tr"), QStringLiteral("de"), QStringLiteral("gb")}.contains(value) || m_keyboardLayout == value) return;
    m_keyboardLayout = value; saveSettings(); emit preferencesChanged();
}
QString ShellController::systemSummary() const {
    return QSysInfo::prettyProductName() + QStringLiteral(" · ") + QSysInfo::currentCpuArchitecture()
        + QStringLiteral(" · ") + QSysInfo::kernelVersion();
}
bool ShellController::openSystemTool(const QString &tool)
{
    const QMap<QString, QStringList> tools = {
        {QStringLiteral("network"), {QStringLiteral("nm-connection-editor")}},
        {QStringLiteral("bluetooth"), {QStringLiteral("blueman-manager")}},
        {QStringLiteral("sound"), {QStringLiteral("pavucontrol")}},
        {QStringLiteral("display"), {QStringLiteral("wdisplays")}},
        {QStringLiteral("input"), {QStringLiteral("rpd-preferences"), QStringLiteral("lxqt-config-input")}}
    };
    if (m_mockMode) { emit actionFailed(tr("System tools are available in the Debian session.")); return false; }
    for (const auto &program : tools.value(tool)) if (startDesktopProcess(program)) return true;
    emit actionFailed(tr("The %1 settings tool is not installed on this device.").arg(tool));
    return false;
}
void ShellController::openSettingsPage(const QString &page) {
    if (m_previewMode && m_mockMode) { openSettings(); return; }
    if (!startDesktopProcess(QStringLiteral("fidan-settings"), {QStringLiteral("--page"), page}))
        emit actionFailed(tr("System settings are unavailable."));
}
