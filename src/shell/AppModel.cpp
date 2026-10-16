// SPDX-License-Identifier: GPL-3.0-or-later
#include "AppModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include "../common/DesktopLaunch.h"
#include <QRegularExpression>
#include <QSettings>
#include <QSet>
#include <QStandardPaths>

#include <algorithm>
#include <utility>

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
        {ExecRole, "exec"}
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

    QString command = m_visibleEntries.at(row).exec;
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
    rebuildVisibleEntries();
}

void AppModel::rebuildVisibleEntries()
{
    beginResetModel();
    if (m_filter.trimmed().isEmpty()) {
        m_visibleEntries = m_entries;
    } else {
        const QString needle = m_filter.trimmed();
        m_visibleEntries.clear();
        for (const Entry &entry : std::as_const(m_entries)) {
            if (entry.name.contains(needle, Qt::CaseInsensitive)
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

