// SPDX-License-Identifier: GPL-3.0-or-later
#include "DesktopModel.h"

#include <QDesktopServices>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QProcess>
#include "../common/DesktopLaunch.h"
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>

DesktopModel::DesktopModel(QObject *parent)
    : DesktopModel(QString(), parent)
{
}

DesktopModel::DesktopModel(const QString &directory, QObject *parent)
    : QAbstractListModel(parent)
    , m_directory(directory)
{
    if (m_directory.isEmpty()) {
        QSettings settings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                               + QStringLiteral("/fidan/shell.conf"), QSettings::IniFormat);
        const QString saved = settings.value(QStringLiteral("desktop/sortMode"), m_sortMode).toString();
        if (saved == QStringLiteral("Type") || saved == QStringLiteral("Date Modified")) m_sortMode = saved;
    }
    QDir().mkpath(desktopPath());
    m_watcher.addPath(desktopPath());
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &DesktopModel::refresh);
    refresh();
}

int DesktopModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant DesktopModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) return {};
    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case NameRole: return entry.name;
    case IconRole: return entry.icon;
    case PathRole: return entry.path;
    case DirectoryRole: return entry.directory;
    case DesktopFileRole: return entry.desktopFile;
    default: return {};
    }
}

QHash<int, QByteArray> DesktopModel::roleNames() const
{
    return {{NameRole, "name"}, {IconRole, "iconName"}, {PathRole, "path"},
            {DirectoryRole, "isDirectory"}, {DesktopFileRole, "isDesktopFile"}};
}

QString DesktopModel::desktopPath() const
{
    if (!m_directory.isEmpty()) return m_directory;
    const QString standard = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    return standard.isEmpty() ? QDir::home().filePath(QStringLiteral("Desktop")) : standard;
}

void DesktopModel::refresh()
{
    QList<Entry> entries;
    const QFileInfoList files = QDir(desktopPath()).entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &file : files) {
        Entry entry;
        entry.name = file.fileName();
        entry.path = file.absoluteFilePath();
        entry.directory = file.isDir();
        entry.desktopFile = file.suffix().compare(QStringLiteral("desktop"), Qt::CaseInsensitive) == 0;
        entry.modified = file.lastModified().toMSecsSinceEpoch();
        entry.suffix = file.suffix().toLower();
        entry.icon = entry.directory ? QStringLiteral("folder") : QStringLiteral("file");
        if (entry.desktopFile) {
            QSettings desktop(entry.path, QSettings::IniFormat);
            desktop.beginGroup(QStringLiteral("Desktop Entry"));
            entry.name = desktop.value(QStringLiteral("Name"), entry.name).toString();
            entry.icon = desktop.value(QStringLiteral("Icon"), QStringLiteral("application-x-executable")).toString();
        }
        entries.append(entry);
    }
    std::sort(entries.begin(), entries.end(), [this](const Entry &left, const Entry &right) {
        if (left.directory != right.directory) return left.directory;
        if (m_sortMode == QStringLiteral("Date Modified") && left.modified != right.modified)
            return left.modified > right.modified;
        if (m_sortMode == QStringLiteral("Type") && left.suffix != right.suffix)
            return left.suffix < right.suffix;
        const int names = left.name.compare(right.name, Qt::CaseInsensitive);
        return names == 0 ? left.path < right.path : names < 0;
    });
    beginResetModel();
    m_entries = entries;
    endResetModel();
}

void DesktopModel::setSortMode(const QString &mode)
{
    if (mode == m_sortMode || (mode != QStringLiteral("Name")
        && mode != QStringLiteral("Type") && mode != QStringLiteral("Date Modified"))) return;
    m_sortMode = mode;
    if (m_directory.isEmpty()) {
        QSettings settings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                               + QStringLiteral("/fidan/shell.conf"), QSettings::IniFormat);
        settings.setValue(QStringLiteral("desktop/sortMode"), mode);
    }
    refresh();
    emit sortModeChanged();
}

QStringList DesktopModel::paths() const
{
    QStringList result;
    for (const auto &entry : m_entries) result.append(entry.path);
    return result;
}

QStringList DesktopModel::dragUrls(const QStringList &paths) const
{
    QStringList urls;
    for (const auto &entry : m_entries)
        if (paths.contains(entry.path)) urls.append(QUrl::fromLocalFile(entry.path).toString(QUrl::FullyEncoded));
    return urls;
}

bool DesktopModel::canDrop(const QList<QUrl> &urls, const QString &destination) const
{
    const QFileInfo folder(destination);
    const QString canonicalDestination = folder.canonicalFilePath();
    if (urls.isEmpty() || !folder.isDir() || canonicalDestination.isEmpty()) return false;
    for (const auto &url : urls) {
        if (!url.isLocalFile() || (!url.host().isEmpty() && url.host() != QStringLiteral("localhost"))) return false;
        const QFileInfo source(url.toLocalFile());
        if ((!source.exists() && !source.isSymbolicLink()) || source.fileName().isEmpty()) return false;
        const QString canonicalSource = source.canonicalFilePath();
        if (source.isDir() && !source.isSymbolicLink()
            && (canonicalSource == canonicalDestination || canonicalDestination.startsWith(canonicalSource + QLatin1Char('/')))) return false;
        const QFileInfo target(QDir(destination).filePath(source.fileName()));
        if (target.exists() || target.isSymbolicLink()) return false;
    }
    return true;
}

bool DesktopModel::open(int row)
{
    if (row < 0 || row >= m_entries.size()) return false;
    const Entry &entry = m_entries.at(row);
    if (entry.directory) {
        if (startDesktopProcess(QStringLiteral("fidan-files"), {entry.path})) return true;
        return startDesktopProcess(QStringLiteral("pcmanfm-qt"), {entry.path});
    }
    if (!entry.desktopFile) return openDesktopFile(entry.path);

    return launchDesktopShortcut(entry.path);
}
