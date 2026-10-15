// SPDX-License-Identifier: GPL-3.0-or-later
#include "ProcessModel.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <pwd.h>
#include <unistd.h>

ProcessModel::ProcessModel(QObject *parent)
    : QAbstractListModel(parent)
{
    refresh();
}

int ProcessModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visible.size();
}

QVariant ProcessModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size()) return {};
    const Entry &entry = m_entries.at(m_visible.at(index.row()));
    switch (role) {
    case PidRole: return entry.pid;
    case NameRole: return entry.name;
    case UserRole: return entry.user;
    case CpuRole: return entry.cpu;
    case MemoryRole: return entry.memory;
    case MemoryBytesRole: return QVariant::fromValue(entry.memoryBytes);
    case StateRole: return entry.state;
    case CommandRole: return entry.command;
    default: return {};
    }
}

QHash<int, QByteArray> ProcessModel::roleNames() const
{
    return {{PidRole, "pid"}, {NameRole, "name"}, {UserRole, "user"}, {CpuRole, "cpu"},
            {MemoryRole, "memory"}, {MemoryBytesRole, "memoryBytes"}, {StateRole, "state"},
            {CommandRole, "command"}};
}

void ProcessModel::setFilter(const QString &filter)
{
    if (m_filter == filter) return;
    m_filter = filter;
    rebuildVisible();
    emit filterChanged();
}

QPair<quint64, quint64> ProcessModel::systemTicks()
{
    QFile file(QStringLiteral("/proc/stat"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    const QStringList fields = QString::fromUtf8(file.readLine()).simplified().split(QLatin1Char(' '));
    quint64 total = 0;
    for (int i = 1; i < fields.size(); ++i) total += fields.at(i).toULongLong();
    const quint64 idle = fields.value(4).toULongLong() + fields.value(5).toULongLong();
    return {total, idle};
}

quint64 ProcessModel::totalMemoryBytes()
{
    QFile file(QStringLiteral("/proc/meminfo"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return 0;
    for (const QByteArray &line : file.readAll().split('\n')) {
        if (line.startsWith("MemTotal:")) {
            return QString::fromUtf8(line).split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts)
                       .value(1).toULongLong() * 1024ULL;
        }
    }
    return 0;
}

bool ProcessModel::readProcess(int pid, Entry &entry)
{
    QFile stat(QStringLiteral("/proc/%1/stat").arg(pid));
    if (!stat.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    const QString line = QString::fromUtf8(stat.readAll());
    const qsizetype open = line.indexOf(QLatin1Char('('));
    const qsizetype close = line.lastIndexOf(QLatin1Char(')'));
    if (open < 0 || close <= open) return false;
    const QStringList fields = line.mid(close + 2).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (fields.size() < 22) return false;

    entry.pid = pid;
    entry.name = line.mid(open + 1, close - open - 1);
    entry.state = fields.at(0);
    entry.processTicks = fields.at(11).toULongLong() + fields.at(12).toULongLong();
    entry.memoryBytes = fields.at(21).toULongLong() * static_cast<quint64>(sysconf(_SC_PAGESIZE));

    QFile command(QStringLiteral("/proc/%1/cmdline").arg(pid));
    if (command.open(QIODevice::ReadOnly)) {
        QByteArray data = command.readAll();
        const QString executable = QFileInfo(QString::fromLocal8Bit(data.split('\0').value(0))).fileName();
        if (!executable.isEmpty()) entry.name = executable;
        data.replace('\0', ' ');
        entry.command = QString::fromUtf8(data).trimmed();
    }
    if (entry.command.isEmpty()) entry.command = QStringLiteral("[%1]").arg(entry.name);

    QFile status(QStringLiteral("/proc/%1/status").arg(pid));
    uid_t uid = 0;
    if (status.open(QIODevice::ReadOnly | QIODevice::Text)) {
        for (const QByteArray &statusLine : status.readAll().split('\n')) {
            if (statusLine.startsWith("Uid:")) {
                uid = static_cast<uid_t>(QString::fromUtf8(statusLine).split(
                    QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts).value(1).toUInt());
                break;
            }
        }
    }
    const passwd *account = getpwuid(uid);
    entry.user = account ? QString::fromLocal8Bit(account->pw_name) : QString::number(uid);
    return true;
}

void ProcessModel::refresh()
{
    const auto [systemTicks, idleTicks] = ProcessModel::systemTicks();
    const quint64 systemDelta = systemTicks > m_previousSystemTicks ? systemTicks - m_previousSystemTicks : 0;
    const quint64 idleDelta = idleTicks > m_previousIdleTicks ? idleTicks - m_previousIdleTicks : 0;
    const quint64 memoryTotal = totalMemoryBytes();
    const int cpuCount = std::max(1L, sysconf(_SC_NPROCESSORS_ONLN));
    QList<Entry> next;
    QHash<int, quint64> nextTicks;

    const QFileInfoList processDirs = QDir(QStringLiteral("/proc")).entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
    for (const QFileInfo &directory : processDirs) {
        bool numeric = false;
        const int pid = directory.fileName().toInt(&numeric);
        if (!numeric) continue;
        Entry entry;
        if (!readProcess(pid, entry)) continue;
        const quint64 previous = m_previousProcessTicks.value(pid, entry.processTicks);
        const quint64 processDelta = entry.processTicks >= previous ? entry.processTicks - previous : 0;
        entry.cpu = systemDelta > 0
            ? std::min(100.0 * cpuCount, 100.0 * cpuCount * static_cast<double>(processDelta) / systemDelta)
            : 0.0;
        entry.memory = memoryTotal > 0 ? 100.0 * static_cast<double>(entry.memoryBytes) / memoryTotal : 0.0;
        nextTicks.insert(pid, entry.processTicks);
        next.append(entry);
    }
    std::sort(next.begin(), next.end(), [](const Entry &left, const Entry &right) {
        if (!qFuzzyCompare(left.cpu + 1.0, right.cpu + 1.0)) return left.cpu > right.cpu;
        return left.memoryBytes > right.memoryBytes;
    });

    beginResetModel();
    m_entries = next;
    m_previousProcessTicks = nextTicks;
    m_previousSystemTicks = systemTicks;
    m_previousIdleTicks = idleTicks;
    m_visible.clear();
    const QString needle = m_filter.trimmed();
    for (int row = 0; row < m_entries.size(); ++row) {
        const Entry &entry = m_entries.at(row);
        if (needle.isEmpty() || entry.name.contains(needle, Qt::CaseInsensitive)
            || entry.command.contains(needle, Qt::CaseInsensitive)
            || QString::number(entry.pid).contains(needle)) m_visible.append(row);
    }
    endResetModel();

    m_totalCpu = systemDelta > 0
        ? 100.0 * static_cast<double>(systemDelta - std::min(systemDelta, idleDelta)) / systemDelta
        : 0.0;
    QFile meminfo(QStringLiteral("/proc/meminfo"));
    quint64 available = 0;
    if (meminfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        for (const QByteArray &line : meminfo.readAll().split('\n')) {
            if (line.startsWith("MemAvailable:")) {
                available = QString::fromUtf8(line).split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts)
                                .value(1).toULongLong() * 1024ULL;
                break;
            }
        }
    }
    m_totalMemory = memoryTotal > 0 ? 100.0 * (memoryTotal - available) / memoryTotal : 0;
    emit totalsChanged();
}

void ProcessModel::rebuildVisible()
{
    beginResetModel();
    m_visible.clear();
    const QString needle = m_filter.trimmed();
    for (int row = 0; row < m_entries.size(); ++row) {
        const Entry &entry = m_entries.at(row);
        if (needle.isEmpty() || entry.name.contains(needle, Qt::CaseInsensitive)
            || entry.command.contains(needle, Qt::CaseInsensitive)
            || QString::number(entry.pid).contains(needle)) m_visible.append(row);
    }
    endResetModel();
}

bool ProcessModel::terminateProcess(int pid, bool force)
{
    if (pid <= 1 || pid == static_cast<int>(getpid())) {
        setError(tr("This system process cannot be stopped."));
        return false;
    }
    if (::kill(static_cast<pid_t>(pid), force ? SIGKILL : SIGTERM) != 0) {
        setError(errno == EPERM ? tr("You do not have permission to stop this process.")
                                : tr("The process could not be stopped."));
        return false;
    }
    setError({});
    return true;
}

void ProcessModel::setError(const QString &error)
{
    if (m_errorText == error) return;
    m_errorText = error;
    emit errorTextChanged();
}
