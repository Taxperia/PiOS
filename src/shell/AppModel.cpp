// SPDX-License-Identifier: GPL-3.0-or-later
#include "AppModel.h"
#include "../common/FileIcon.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include "../common/DesktopLaunch.h"
#include <QRegularExpression>
#include <QSettings>
#include <QSet>
#include <QStandardPaths>
#include <QXmlStreamReader>
#include <QLocale>
#ifdef Q_OS_UNIX
#include <pwd.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <utility>

namespace {
QString launcherSettingsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/fidan/shell.conf");
}
}

AppModel::AppModel(QObject *parent)
    : QAbstractListModel(parent)
{
    refresh();
}

int AppModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visibleEntries.size();
}

QVariant AppModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleEntries.size()) {
        return {};
    }
    const Entry &entry = m_visibleEntries.at(index.row());
    switch (role) {
    case NameRole: return entry.name;
    case IconRole: return entry.icon;
    case DesktopIdRole: return entry.desktopId;
    case CommentRole: return entry.comment;
    case ExecRole: return entry.exec;
    case PinnedRole: return m_pinnedIds.contains(entry.desktopId);
    default: return {};
    }
}

QHash<int, QByteArray> AppModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {IconRole, "iconName"},
        {DesktopIdRole, "desktopId"},
        {CommentRole, "comment"},
        {ExecRole, "exec"},
        {PinnedRole, "pinned"}
    };
}

void AppModel::setFilter(const QString &filter)
{
    if (m_filter == filter) {
        return;
    }
    m_filter = filter;
    rebuildVisibleEntries();
    emit filterChanged();
}

bool AppModel::launch(int row)
{
    if (row < 0 || row >= m_visibleEntries.size()) {
        return false;
    }

    return launchEntry(m_visibleEntries.at(row));
}

bool AppModel::launchDesktopId(const QString &desktopId)
{
    for (const auto &entry : std::as_const(m_entries))
        if (entry.desktopId == desktopId) return launchEntry(entry);
    return false;
}

bool AppModel::launchEntry(const Entry &entry)
{
    QString command = entry.exec;
    command.remove(QRegularExpression(QStringLiteral("\\s+%[fFuUdDnNickvm]")));
    command.remove(QRegularExpression(QStringLiteral("%[fFuUdDnNickvm]")));
    const QStringList parts = QProcess::splitCommand(command.trimmed());
    if (parts.isEmpty()) {
        return false;
    }

    QStringList arguments = parts;
    const QString program = arguments.takeFirst();
    return startDesktopProcess(program, arguments);
}

bool AppModel::addToDesktop(int row)
{
    if (row < 0 || row >= m_visibleEntries.size()) return false;
    const Entry &entry = m_visibleEntries.at(row);
    if (entry.sourcePath.isEmpty() || !QFileInfo::exists(entry.sourcePath)) return false;
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopPath.isEmpty()) desktopPath = QDir::home().filePath(QStringLiteral("Desktop"));
    if (!QDir().mkpath(desktopPath)) return false;
    const QString target = QDir(desktopPath).filePath(entry.desktopId);
    if (QFileInfo::exists(target) && !QFile::remove(target)) return false;
    if (!QFile::copy(entry.sourcePath, target)) return false;
    return QFile::setPermissions(target, QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                         | QFileDevice::ExeOwner | QFileDevice::ReadGroup
                                         | QFileDevice::ExeGroup | QFileDevice::ReadOther
                                         | QFileDevice::ExeOther);
}

void AppModel::refresh()
{
    m_entries = discoverApplications();
    QSettings settings(launcherSettingsPath(), QSettings::IniFormat);
    if (settings.contains(QStringLiteral("launcher/pinnedApps"))) {
        m_pinnedIds = settings.value(QStringLiteral("launcher/pinnedApps")).toStringList();
    } else {
        m_pinnedIds.clear();
        const QList<QStringList> defaults {
            {"org.fidan.Files.desktop", "pcmanfm-qt.desktop"},
            {"qterminal.desktop", "org.gnome.Terminal.desktop"},
            {"chromium.desktop", "firefox-esr.desktop", "firefox.desktop"},
            {"org.fidan.Settings.desktop"}, {"org.fidan.Store.desktop"},
            {"vlc.desktop", "org.videolan.VLC.desktop", "org.fidan.Photos.desktop"},
            {"org.fidan.TaskManager.desktop"},
            {"code.desktop", "codium.desktop", "org.fidan.Calculator.desktop"}
        };
        for (const auto &choices : defaults) {
            for (const auto &id : choices) {
                if (std::any_of(m_entries.cbegin(), m_entries.cend(), [&](const Entry &e) { return e.desktopId == id; })) {
                    m_pinnedIds.append(id);
                    break;
                }
            }
        }
        for (const auto &entry : std::as_const(m_entries)) {
            if (m_pinnedIds.size() >= 8) break;
            if (!m_pinnedIds.contains(entry.desktopId)) m_pinnedIds.append(entry.desktopId);
        }
    }
    readRecentFiles();
    rebuildVisibleEntries();
    emit launcherDataChanged();
}

void AppModel::setCategory(const QString &category)
{
    if (m_category == category) return;
    m_category = category;
    rebuildVisibleEntries();
    emit categoryChanged();
}

QVariantList AppModel::pinnedApplications() const
{
    QVariantList result;
    for (const auto &id : m_pinnedIds) {
        for (const auto &entry : m_entries) {
            if (entry.desktopId == id) {
                result.append(QVariantMap{{"name", entry.name}, {"iconName", entry.icon}, {"desktopId", id}});
                break;
            }
        }
    }
    return result;
}

bool AppModel::togglePinned(const QString &desktopId)
{
    if (!std::any_of(m_entries.cbegin(), m_entries.cend(), [&](const Entry &e) { return e.desktopId == desktopId; })) return false;
    if (m_pinnedIds.contains(desktopId)) m_pinnedIds.removeAll(desktopId);
    else m_pinnedIds.append(desktopId);
    QSettings settings(launcherSettingsPath(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("launcher/pinnedApps"), m_pinnedIds);
    rebuildVisibleEntries();
    emit launcherDataChanged();
    return true;
}

QString AppModel::userHandle() const
{
    const QString login = qEnvironmentVariable("USER", qEnvironmentVariable("USERNAME", "pios"));
    return login + QStringLiteral("@pios");
}

QString AppModel::userName() const
{
#ifdef Q_OS_UNIX
    if (const passwd *account = getpwuid(getuid())) {
        const QString name = QString::fromLocal8Bit(account->pw_gecos).section(',', 0, 0).trimmed();
        if (!name.isEmpty()) return name;
        return QString::fromLocal8Bit(account->pw_name);
    }
#endif
    return userHandle().section('@', 0, 0);
}

void AppModel::readRecentFiles()
{
    m_recentFiles.clear();
    m_recentUrls.clear();
    QFile file(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/recently-used.xbel"));
    if (!file.open(QIODevice::ReadOnly) || file.size() > 2 * 1024 * 1024) return;
    struct Recent { QUrl url; QDateTime modified; };
    QList<Recent> entries;
    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() || xml.name() != QLatin1String("bookmark")) continue;
        const QUrl url(xml.attributes().value("href").toString());
        QDateTime modified = QDateTime::fromString(xml.attributes().value("modified").toString(), Qt::ISODate);
        const QDateTime visited = QDateTime::fromString(xml.attributes().value("visited").toString(), Qt::ISODate);
        if (visited > modified) modified = visited;
        bool privateEntry = false;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name() == QLatin1String("private")) privateEntry = true;
            if (xml.isEndElement() && xml.name() == QLatin1String("bookmark")) break;
        }
        if (!url.isLocalFile() || privateEntry) continue;
        const QFileInfo info(url.toLocalFile());
        if (!info.isFile() || !info.isReadable() || info.suffix().compare("desktop", Qt::CaseInsensitive) == 0) continue;
        entries.append({QUrl::fromLocalFile(info.absoluteFilePath()), modified});
    }
    if (xml.hasError()) return;
    std::stable_sort(entries.begin(), entries.end(), [](const Recent &a, const Recent &b) { return a.modified > b.modified; });
    QSet<QString> seen;
    const QDate today = QDate::currentDate();
    for (const auto &entry : std::as_const(entries)) {
        const QFileInfo info(entry.url.toLocalFile());
        if (seen.contains(info.absoluteFilePath())) continue;
        seen.insert(info.absoluteFilePath());
        const QDateTime time = entry.modified.toLocalTime();
        const QString date = time.date() == today ? time.toString("HH:mm")
            : time.date() == today.addDays(-1) ? tr("Yesterday") : QLocale().toString(time.date(), "d MMM");
        QString folder = info.absolutePath();
        if (folder == QDir::homePath() || folder.startsWith(QDir::homePath() + '/')) folder.replace(0, QDir::homePath().size(), "~");
        m_recentFiles.append(QVariantMap{{"name", info.fileName()}, {"folder", folder},
            {"iconName", fileIconName(info)}, {"date", date}, {"row", m_recentUrls.size()}});
        m_recentUrls.append(entry.url);
        if (m_recentUrls.size() >= 20) break;
    }
}

bool AppModel::openRecentFile(int row)
{
    if (row < 0 || row >= m_recentUrls.size()) return false;
    const QFileInfo file(m_recentUrls.at(row).toLocalFile());
    if (!file.isFile() || !file.isReadable() || file.suffix().compare("desktop", Qt::CaseInsensitive) == 0) return false;
    return QDesktopServices::openUrl(m_recentUrls.at(row));
}

void AppModel::rebuildVisibleEntries()
{
    beginResetModel();
    if (m_filter.trimmed().isEmpty() && m_category.isEmpty()) {
        m_visibleEntries = m_entries;
    } else {
        const QString needle = m_filter.trimmed();
        m_visibleEntries.clear();
        for (const Entry &entry : std::as_const(m_entries)) {
            if (m_category == QLatin1String("Pinned") ? !m_pinnedIds.contains(entry.desktopId)
                : !m_category.isEmpty() && !entry.categories.contains(m_category)) continue;
            if (needle.isEmpty() || entry.name.contains(needle, Qt::CaseInsensitive)
                || entry.comment.contains(needle, Qt::CaseInsensitive)
                || entry.desktopId.contains(needle, Qt::CaseInsensitive)
                || entry.exec.contains(needle, Qt::CaseInsensitive)) {
                m_visibleEntries.append(entry);
            }
        }
    }
    endResetModel();
}

QList<AppModel::Entry> AppModel::discoverApplications() const
{
    QList<Entry> result;
    QStringList roots = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    roots.append(QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
                     .filePath(QStringLiteral("flatpak/exports/share/applications")));
    roots.append(QStringLiteral("/var/lib/flatpak/exports/share/applications"));
    roots.append(QStringLiteral("/usr/share/applications"));
    roots.removeDuplicates();

    QSet<QString> seen;
    for (const QString &root : std::as_const(roots)) {
        const QDir directory(root);
        const QFileInfoList files = directory.entryInfoList({QStringLiteral("*.desktop")}, QDir::Files);
        for (const QFileInfo &file : files) {
            if (seen.contains(file.fileName())) {
                continue;
            }

            QSettings desktop(file.absoluteFilePath(), QSettings::IniFormat);
            desktop.beginGroup(QStringLiteral("Desktop Entry"));
            if (desktop.value(QStringLiteral("Type")).toString() != QStringLiteral("Application")
                || desktop.value(QStringLiteral("NoDisplay"), false).toBool()
                || desktop.value(QStringLiteral("Hidden"), false).toBool()) {
                desktop.endGroup();
                continue;
            }

            Entry entry;
            entry.name = desktop.value(QStringLiteral("Name")).toString();
            entry.comment = desktop.value(QStringLiteral("Comment")).toString();
            entry.icon = desktop.value(QStringLiteral("Icon")).toString();
            entry.exec = desktop.value(QStringLiteral("Exec")).toString();
            entry.desktopId = file.fileName();
            if (entry.icon.trimmed().isEmpty()) entry.icon = file.completeBaseName();
            entry.sourcePath = file.absoluteFilePath();
            entry.categories = desktop.value(QStringLiteral("Categories")).toString().split(';', Qt::SkipEmptyParts);
            desktop.endGroup();

            if (!entry.name.isEmpty() && !entry.exec.isEmpty()) {
                result.append(entry);
                seen.insert(file.fileName());
            }
        }
    }

    if (result.isEmpty()) {
        result = {
            {QStringLiteral("Web Browser"), QStringLiteral("web-browser"), QStringLiteral("chromium.desktop"), QStringLiteral("Browse the web"), QStringLiteral("chromium"), {}},
            {QStringLiteral("Files"), QStringLiteral("system-file-manager"), QStringLiteral("org.fidan.Files.desktop"), QStringLiteral("Browse files"), QStringLiteral("fidan-files"), {}},
            {QStringLiteral("Terminal"), QStringLiteral("utilities-terminal"), QStringLiteral("qterminal.desktop"), QStringLiteral("Use the command line"), QStringLiteral("qterminal"), {}},
            {QStringLiteral("Text Editor"), QStringLiteral("accessories-text-editor"), QStringLiteral("featherpad.desktop"), QStringLiteral("Edit text files"), QStringLiteral("featherpad"), {}}
        };
    }

    std::sort(result.begin(), result.end(), [](const Entry &left, const Entry &right) {
        return QString::localeAwareCompare(left.name, right.name) < 0;
    });
    return result;
}

