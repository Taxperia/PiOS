// SPDX-License-Identifier: GPL-3.0-or-later
#include "SettingsData.h"
#include "../common/DesktopLaunch.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLocale>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QSysInfo>
#include <QThread>
#include <QTimeZone>
#include <QUrl>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QImageReader>
#include <QSet>
#include <algorithm>
#include <QProcessEnvironment>

namespace {
QString read(const QString &path) { QFile f(path); return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString(); }
const QMap<QString, QStringList> tools{
    {"credentials", {"keepassxc", "seahorse"}},
    {"network", {"nm-connection-editor"}}, {"storage", {"gnome-disks", "partitionmanager", "gparted"}},
    {"usage", {"baobab", "filelight"}}, {"users", {"lxqt-admin-user", "users-admin"}},
    {"backup", {"deja-dup", "pika-backup"}}, {"input", {"rpd-preferences", "lxqt-config-input"}}
};
}

SettingsData::SettingsData(bool mock, QObject *parent) : QObject(parent), m_mock(mock) {
    m_desktop=new DesktopSettings(mock,this);
    connect(&m_network, &QProcess::stateChanged, this, [this] { emit networkChanged(); });
    connect(&m_network, qOverload<int,QProcess::ExitStatus>(&QProcess::finished), this, &SettingsData::networkFinished);
    connect(&m_network, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) { m_networkTimeout.stop(); m_networkError = tr("NetworkManager could not be started."); emit networkChanged(); }
    });
    m_networkTimeout.setSingleShot(true); m_networkTimeout.setInterval(30000);
    connect(&m_networkTimeout, &QTimer::timeout, this, [this] { m_networkError = tr("The network operation timed out."); m_network.kill(); });
    connect(&m_action, &QProcess::stateChanged, this, [this] { emit actionChanged(); });
    connect(&m_action, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) { m_actionTimeout.stop(); m_actionError = tr("The system service could not be started."); emit actionChanged(); }
    });
    connect(&m_action, qOverload<int,QProcess::ExitStatus>(&QProcess::finished), this, [this](int code, QProcess::ExitStatus status) {
        m_actionTimeout.stop();
        if (code || status != QProcess::NormalExit) m_actionError = QString::fromUtf8(m_action.readAllStandardError()).trimmed();
        if ((code || status != QProcess::NormalExit) && m_actionError.isEmpty()) m_actionError = tr("The system did not accept this change.");
        refreshSystem(); emit actionChanged();
    });
    m_actionTimeout.setSingleShot(true); m_actionTimeout.setInterval(30000);
    connect(&m_actionTimeout, &QTimer::timeout, this, [this] { m_action.kill(); });
    refreshSystem();
}
SettingsData::~SettingsData() {
    disconnect(&m_network, nullptr, this, nullptr); disconnect(&m_action, nullptr, this, nullptr);
    for (auto *p : {&m_network, &m_action}) if (p->state() != QProcess::NotRunning) { p->kill(); p->waitForFinished(1000); }
}
void SettingsData::refreshSystem() {
    QString cpu;
    for (const auto &line : read("/proc/cpuinfo").split('\n')) if (line.startsWith("model name") || line.startsWith("Model")) { cpu = line.section(':',1).trimmed(); break; }
    const auto memoryMatch = QRegularExpression("MemTotal:\\s+(\\d+)").match(read("/proc/meminfo"));
    const qint64 memory = memoryMatch.hasMatch() ? memoryMatch.captured(1).toLongLong()*1024 : 0;
    m_info = {{"host", QSysInfo::machineHostName()}, {"cpu", cpu.isEmpty() ? QSysInfo::currentCpuArchitecture() : cpu},
        {"cores", QThread::idealThreadCount()}, {"memory", memory ? QLocale().formattedDataSize(memory) : tr("Unavailable")},
        {"architecture", QSysInfo::currentCpuArchitecture()}, {"os", QSysInfo::prettyProductName()}, {"kernel", QSysInfo::kernelVersion()},
        {"session", QGuiApplication::platformName() == "wayland" ? "Wayland" : QGuiApplication::platformName()},
        {"user", qEnvironmentVariable("USER")}, {"home", QDir::homePath()}, {"locale", QLocale::system().language()==QLocale::C ? QStringLiteral("C.UTF-8") : QLocale::system().nativeLanguageName() + " · " + QLocale::system().name()},
        {"time", QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm")}, {"timezone", QString::fromUtf8(QTimeZone::systemTimeZoneId())}, {"ntpAvailable", false}, {"ntpEnabled", false}};
    if (!m_mock) {
        QDBusInterface clock("org.freedesktop.timedate1", "/org/freedesktop/timedate1", "org.freedesktop.timedate1", QDBusConnection::systemBus());
        if (clock.isValid()) { m_info["ntpAvailable"] = clock.property("CanNTP"); m_info["ntpEnabled"] = clock.property("NTP"); }
    }
    m_volumes.clear(); QSet<QByteArray> seen;
    auto volumes = QStorageInfo::mountedVolumes();
    std::stable_sort(volumes.begin(), volumes.end(), [](const QStorageInfo &a, const QStorageInfo &b) { return a.rootPath() == "/" && b.rootPath() != "/"; });
    for (const auto &v : volumes) {
        const QString mount = v.rootPath();
        QByteArray deviceKey=v.device(); if(v.fileSystemType()=="9p" || deviceKey.isEmpty()) deviceKey += ':' + mount.toUtf8();
        if (!v.isReady() || !v.isValid() || v.bytesTotal() <= 0 || seen.contains(deviceKey)) continue;
        if (mount != "/" && !mount.startsWith("/home") && !mount.startsWith("/media/") && !mount.startsWith("/boot") && !QRegularExpression("^/mnt/[a-z]$").match(mount).hasMatch()) continue;
        seen.insert(deviceKey); const qint64 used = qMax(qint64(0), v.bytesTotal()-v.bytesFree());
        m_volumes.append(QVariantMap{{"mount", mount}, {"name", mount == "/" ? tr("System disk") : v.displayName()}, {"filesystem", QString::fromUtf8(v.fileSystemType())},
            {"used",used}, {"total",v.bytesTotal()}, {"free",v.bytesAvailable()}, {"percent", double(used)*100/v.bytesTotal()},
            {"usedText",QLocale().formattedDataSize(used)}, {"totalText",QLocale().formattedDataSize(v.bytesTotal())}, {"freeText",QLocale().formattedDataSize(v.bytesAvailable())}});
    }
    emit systemChanged();
}
QVariantList SettingsData::wallpapers() const {
    QVariantList result; QSet<QString> seen;
    QStringList roots{"/usr/share/backgrounds/fidan", QStringLiteral(FIDAN_SOURCE_DIR "/artwork/wallpapers")};
    for (const auto &root : roots) for (const auto &folder : {root, root + "/collection"}) {
        for (const auto &file : QDir(folder).entryInfoList({"*.png", "*.jpg", "*.jpeg", "*.svg", "*.webp"}, QDir::Files, QDir::Name)) {
            if (file.fileName() == "fidan-mountain-dawn.png" || seen.contains(file.fileName())) continue;
            seen.insert(file.fileName()); QString name = file.completeBaseName(); name.remove("fidan-"); name.remove("-4k"); name.replace('-', ' ');
            result.append(QVariantMap{{"url",QUrl::fromLocalFile(file.absoluteFilePath())}, {"name",name}});
        }
    }
    return result;
}
QString SettingsData::licenseText() const { return read("/usr/share/common-licenses/GPL-3"); }
bool SettingsData::toolAvailable(const QString &key) const { for (const auto &tool : tools.value(key)) if (!QStandardPaths::findExecutable(tool).isEmpty()) return true; return false; }
bool SettingsData::openTool(const QString &key) {
    if (m_mock) return false;
    for (const auto &tool : tools.value(key)) if (startDesktopProcess(tool)) return true;
    m_actionError = tr("This system tool is not installed. You can install it from the application store."); emit actionChanged(); return false;
}
bool SettingsData::openHelp() {
    for (const auto &path : {QStringLiteral("/usr/share/doc/fidan-shell/daily-desktop.md"), QStringLiteral(FIDAN_SOURCE_DIR "/docs/daily-desktop.md")})
        if (QFileInfo::exists(path)) return startDesktopProcess("xdg-open", {path});
    return false;
}
void SettingsData::setNtp(bool enabled) {
    if (m_mock || actionBusy() || !m_info.value("ntpAvailable").toBool()) return;
    m_actionError.clear(); m_action.start("timedatectl", {"set-ntp", enabled ? "true" : "false"}); m_actionTimeout.start(); emit actionChanged();
}
QStringList SettingsData::splitFields(const QString &line) {
    QStringList result; QString field; bool escaped = false;
    for (QChar c : line) {
        if (escaped) { field += c; escaped = false; }
        else if (c == '\\') escaped = true;
        else if (c == ':') { result.append(field); field.clear(); }
        else field += c;
    }
    if (escaped) field += '\\'; result.append(field); return result;
}
void SettingsData::refreshNetwork(bool rescan) {
    if (networkBusy() || m_mock) return;
    m_rescan = rescan; m_networkError.clear(); m_networks.clear(); m_connection.clear(); m_wifiDevice.clear(); m_connectionDevice.clear();
    networkStart("devices", {"-t", "-f", "DEVICE,TYPE,STATE,CONNECTION", "device", "status"});
}
void SettingsData::networkStart(const QString &phase, const QStringList &args) {
    m_phase = phase; auto env = QProcessEnvironment::systemEnvironment(); env.insert("LC_ALL", "C"); m_network.setProcessEnvironment(env);
    m_network.start("nmcli", args); m_networkTimeout.start(); emit networkChanged();
}
void SettingsData::queryConnection() {
    if (m_connectionDevice.isEmpty()) { emit networkChanged(); return; }
    networkStart("details", {"-t", "device", "show", m_connectionDevice});
}
void SettingsData::networkFinished(int code, QProcess::ExitStatus status) {
    m_networkTimeout.stop(); const auto phase = m_phase;
    const auto output = QString::fromUtf8(m_network.readAllStandardOutput());
    const auto error = QString::fromUtf8(m_network.readAllStandardError()).trimmed();
    if (code || status != QProcess::NormalExit) {
        m_networkError = error.isEmpty() ? tr("The network operation failed.") : error;
        emit networkChanged(); return;
    }
    if (phase == "devices") {
        for (const auto &line : output.split('\n',Qt::SkipEmptyParts)) {
            const auto f = splitFields(line); if (f.size() < 4) continue;
            if (f[1] == "wifi" && m_wifiDevice.isEmpty()) m_wifiDevice = f[0];
            if (f[2].startsWith("connected") && (f[1] == "wifi" || (m_connectionDevice.isEmpty() && f[1] == "ethernet"))) m_connectionDevice = f[0];
        }
        if (!m_wifiDevice.isEmpty()) networkStart("scan", {"-t", "-f", "IN-USE,SSID,SIGNAL,FREQ,SECURITY,RATE", "device", "wifi", "list", "ifname", m_wifiDevice, "--rescan", m_rescan ? "yes" : "no"});
        else queryConnection();
    } else if (phase == "scan") {
        QSet<QString> seen;
        for (const auto &line : output.split('\n',Qt::SkipEmptyParts)) {
            const auto f = splitFields(line); if (f.size() < 6 || f[1].isEmpty() || seen.contains(f[1])) continue;
            seen.insert(f[1]); const int mhz = f[3].section(' ',0,0).toInt();
            QVariantMap entry{{"ssid",f[1]}, {"connected",f[0]=="*"}, {"signal",f[2].toInt()}, {"band",mhz>=5900 ? "6 GHz" : mhz>=4900 ? "5 GHz" : "2.4 GHz"}, {"security", f[4]}, {"rate",f[5]}};
            m_networks.append(entry); if (f[0] == "*") m_connection = entry;
        }
        queryConnection();
    } else if (phase == "details") {
        QStringList dns;
        for (const auto &line : output.split('\n',Qt::SkipEmptyParts)) {
            const auto f = splitFields(line); if (f.size() < 2) continue; const auto value = f.mid(1).join(':');
            if (f[0] == "GENERAL.CONNECTION") m_connection["name"] = value;
            if (f[0] == "GENERAL.DEVICE") m_connection["device"] = value;
            if (f[0] == "GENERAL.TYPE") m_connection["type"] = value;
            if (f[0] == "GENERAL.STATE") m_connection["state"] = value;
            if (f[0] == "GENERAL.MTU") m_connection["mtu"] = value;
            if (f[0].startsWith("IP4.ADDRESS")) m_connection["address"] = value;
            if (f[0] == "IP4.GATEWAY") m_connection["gateway"] = value;
            if (f[0].startsWith("IP4.DNS")) dns.append(value);
        }
        if (dns.isEmpty()) for (const auto &line : read("/etc/resolv.conf").split('\n')) {
            if (line.trimmed().startsWith("nameserver ")) dns.append(line.simplified().section(' ',1,1));
        }
        m_connection["dns"] = dns.join(" / ");
        if (QRegularExpression("^[A-Za-z0-9_.:-]{1,64}$").match(m_connectionDevice).hasMatch()) {
            const int speed=read("/sys/class/net/"+m_connectionDevice+"/speed").toInt();
            if (speed>0 && m_connection.value("rate").toString().isEmpty()) m_connection["rate"] = QString::number(speed)+" Mbps";
        }
    } else if (phase == "connect" || phase == "disconnect") { refreshNetwork(); return; }
    emit networkChanged();
}
void SettingsData::connectNetwork(const QString &ssid, const QString &password) {
    if (networkBusy() || m_mock || m_wifiDevice.isEmpty() || ssid.isEmpty()) return;
    bool found = false; for (const auto &n : m_networks) if (n.toMap().value("ssid").toString() == ssid) found = true; if (!found) return;
    if (password.contains(QLatin1Char('\n')) || password.contains(QLatin1Char('\r'))
        || password.contains(QChar::Null) || ssid.contains(QChar::Null)) {
        m_networkError = tr("The Wi-Fi settings are invalid."); emit networkChanged(); return;
    }
    // Password arguments are visible through /proc and process inspection.
    // nmcli's prompt reads the secret from this private pipe instead.
    m_networkError.clear(); QStringList args{"--ask", "--wait", "20", "device", "wifi", "connect", ssid, "ifname", m_wifiDevice};
    networkStart("connect", args);
    if (!password.isEmpty()) m_network.write(password.toUtf8() + '\n');
    m_network.closeWriteChannel();
}
void SettingsData::disconnectNetwork() {
    if (networkBusy() || m_mock || m_wifiDevice.isEmpty()) return;
    m_networkError.clear(); networkStart("disconnect", {"--wait", "15", "device", "disconnect", m_wifiDevice});
}
