// SPDX-License-Identifier: GPL-3.0-or-later
#include "StoreModel.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include "../common/DesktopLaunch.h"
#include <QtConcurrentRun>

StoreModel::StoreModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &StoreModel::collectOperationOutput);
    connect(&m_process, &QProcess::readyReadStandardError, this, &StoreModel::collectOperationOutput);
    connect(&m_process, &QProcess::stateChanged, this, [this] {
        emit busyChanged();
        if (!m_visible.isEmpty()) emit dataChanged(index(0), index(m_visible.size() - 1), {EntryBusyRole});
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        setError(m_process.errorString());
        if (error == QProcess::FailedToStart) {
            m_configuringRemote = false;
            m_activeSourceRow = -1;
            emit busyChanged();
            if (!m_visible.isEmpty()) emit dataChanged(index(0), index(m_visible.size() - 1));
        }
    });
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &StoreModel::finishOperation);
    connect(&m_refresh, &QFutureWatcher<QSet<QString>>::finished, this, [this] {
        const auto installed = m_refresh.result();
        m_refreshing = false;
        for (auto &entry : m_entries)
            if (!installed.contains(QLatin1Char('!') + entry.source))
                entry.installed = installed.contains(entry.source + QLatin1Char(':') + entry.identifier);
        rebuildVisible(); emit busyChanged();
        if (installed.contains(QStringLiteral("!apt")) || installed.contains(QStringLiteral("!flatpak")))
            setError(tr("Some application status could not be checked. Check that APT and Flatpak are available, then refresh."));
        else setStatus(tr("Installed application status refreshed."));
        queryUpdates();
    });
    loadCatalog();
    refresh();
}

StoreModel::~StoreModel()
{
    m_refresh.waitForFinished();
    // The window refuses to close during a package transaction. Queries may
    // be stopped at exit, but a package writer must be allowed to finish.
    m_process.waitForFinished(-1);
    for (auto *query : {&m_aptUpdates, &m_flatpakUpdates}) {
        if (query->state() != QProcess::NotRunning) { query->kill(); query->waitForFinished(); }
    }
}

int StoreModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visible.size();
}

QVariant StoreModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size()) {
        return {};
    }
    const int sourceRow = m_visible.at(index.row());
    const Entry &entry = m_entries.at(sourceRow);
    switch (role) {
    case NameRole: return entry.name;
    case SummaryRole: return entry.summary;
    case CategoryRole: return entry.category;
    case IconRole: return entry.icon;
    case SourceRole: return entry.source;
    case IdentifierRole: return entry.identifier;
    case UpdateAvailableRole: return entry.updateAvailable;
    case InstalledRole: return entry.installed;
    case LaunchableRole: return entry.installed && (entry.source == QStringLiteral("flatpak") || !entry.desktopId.isEmpty());
    case EntryBusyRole: return busy() && sourceRow == m_activeSourceRow;
    default: return {};
    }
}

QHash<int, QByteArray> StoreModel::roleNames() const
{
    return {{NameRole, "name"}, {SummaryRole, "summary"}, {CategoryRole, "category"},
            {IconRole, "iconName"}, {SourceRole, "source"}, {IdentifierRole, "identifier"},
            {InstalledRole, "installed"}, {EntryBusyRole, "entryBusy"}, {UpdateAvailableRole, "updateAvailable"}, {LaunchableRole, "launchable"}};
}

void StoreModel::setFilter(const QString &filter)
{
    if (m_filter == filter) return;
    m_filter = filter;
    rebuildVisible();
    emit filterChanged();
}

void StoreModel::setCategory(const QString &category)
{
    if (m_category == category) return;
    m_category = category;
    rebuildVisible();
    emit categoryChanged();
}

void StoreModel::setInstalledOnly(bool installedOnly)
{
    if (m_installedOnly == installedOnly) return;
    m_installedOnly = installedOnly;
    rebuildVisible();
    emit installedOnlyChanged();
}

void StoreModel::loadCatalog()
{
    QString path = QStringLiteral("/usr/share/fidan/store/catalog.json");
    if (!QFile::exists(path)) {
        path = QStringLiteral(FIDAN_SOURCE_DIR "/data/store/catalog.json");
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(tr("The application catalog could not be opened."));
        return;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(tr("The application catalog is invalid."));
        return;
    }

    static const QRegularExpression aptId(QStringLiteral("^[a-z0-9][a-z0-9+.-]*$"));
    static const QRegularExpression flatpakId(QStringLiteral("^[A-Za-z0-9_-]+(\\.[A-Za-z0-9_-]+)+$"));
    m_entries.clear();
    for (const QJsonValue value : document.object().value(QStringLiteral("apps")).toArray()) {
        const QJsonObject object = value.toObject();
        Entry entry{object.value(QStringLiteral("name")).toString(),
                    object.value(QStringLiteral("summary")).toString(),
                    object.value(QStringLiteral("category")).toString(),
                    object.value(QStringLiteral("icon")).toString(),
                    object.value(QStringLiteral("source")).toString(),
                    object.value(QStringLiteral("id")).toString()};
        entry.desktopId = object.value(QStringLiteral("desktopId")).toString();
        const bool validId = entry.source == QStringLiteral("apt")
            ? aptId.match(entry.identifier).hasMatch()
            : entry.source == QStringLiteral("flatpak") && flatpakId.match(entry.identifier).hasMatch();
        if (!entry.name.isEmpty() && validId) m_entries.append(entry);
    }
    rebuildVisible();
}

void StoreModel::rebuildVisible()
{
    beginResetModel();
    m_visible.clear();
    const QString needle = m_filter.trimmed();
    for (int row = 0; row < m_entries.size(); ++row) {
        const Entry &entry = m_entries.at(row);
        const bool matchesText = needle.isEmpty() || entry.name.contains(needle, Qt::CaseInsensitive)
            || entry.summary.contains(needle, Qt::CaseInsensitive)
            || entry.category.contains(needle, Qt::CaseInsensitive);
        const bool matchesCategory = m_category.isEmpty()
            || entry.category.compare(m_category, Qt::CaseInsensitive) == 0;
        if (matchesText && matchesCategory && (!m_installedOnly || entry.installed) && (!m_updatesOnly || entry.updateAvailable)) {
            m_visible.append(row);
        }
    }
    endResetModel();
}

bool StoreModel::queryInstalled(const Entry &entry) const
{
    QProcess process;
    if (entry.source == QStringLiteral("flatpak")) {
        process.start(QStringLiteral("flatpak"), {QStringLiteral("info"), QStringLiteral("--user"), entry.identifier});
    } else {
        process.start(QStringLiteral("dpkg-query"),
                      {QStringLiteral("-W"), QStringLiteral("-f=${db:Status-Abbrev}"), entry.identifier});
    }
    return process.waitForFinished(2500) && process.exitStatus() == QProcess::NormalExit
        && process.exitCode() == 0
        && (entry.source == QStringLiteral("flatpak")
            || QString::fromUtf8(process.readAllStandardOutput()).startsWith(QStringLiteral("ii")));
}

void StoreModel::refresh()
{
    if (busy()) return;
    m_lastIdentifier.clear(); setError({});
    setStatus(tr("Checking installed applications…"));
    m_refreshing = true;
    m_refresh.setFuture(QtConcurrent::run([] {
        QSet<QString> result;
        const auto query = [&result](const QString &source, const QString &program, const QStringList &args) {
            QProcess process; process.start(program, args);
            if (!process.waitForFinished(5000) || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
                if (process.state() != QProcess::NotRunning) { process.kill(); process.waitForFinished(); }
                result.insert(QLatin1Char('!') + source); return;
            }
            for (const QString &line : QString::fromUtf8(process.readAllStandardOutput()).split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
                if (source == QStringLiteral("apt")) {
                    if (!line.section(QLatin1Char('\t'), 1).startsWith(QStringLiteral("ii"))) continue;
                    result.insert(source + QLatin1Char(':') + line.section(QLatin1Char('\t'), 0, 0).section(QLatin1Char(':'), 0, 0));
                } else result.insert(source + QLatin1Char(':') + line.trimmed());
            }
        };
        query(QStringLiteral("apt"), QStringLiteral("dpkg-query"), {QStringLiteral("-W"), QStringLiteral("-f=${binary:Package}\t${db:Status-Abbrev}\n")});
        query(QStringLiteral("flatpak"), QStringLiteral("flatpak"), {QStringLiteral("list"), QStringLiteral("--user"), QStringLiteral("--app"), QStringLiteral("--columns=application")});
        return result;
    }));
    emit busyChanged();
}

void StoreModel::installOrRemove(int row)
{
    if (busy() || row < 0 || row >= m_visible.size()) return;
    m_activeSourceRow = m_visible.at(row);
    const Entry &entry = m_entries.at(m_activeSourceRow);
    m_removing = entry.installed;
    m_updating = false;
    m_lastIdentifier = entry.identifier; m_lastRemoving = m_removing; m_lastUpdating = false;
    m_progress = -1;
    m_operationOutput.clear();
    emit operationOutputChanged();
    m_errorText.clear();
    emit errorTextChanged();
    setStatus(m_removing ? tr("Removing %1…").arg(entry.name) : tr("Installing %1…").arg(entry.name));

    m_configuringRemote = entry.source == QStringLiteral("flatpak") && !m_removing;
    startPackageOperation();
}

void StoreModel::startPackageOperation()
{
    m_phaseOutput.clear();
    const Entry &entry = m_entries.at(m_activeSourceRow);
    QString program;
    QStringList arguments;
    if (entry.source == QStringLiteral("flatpak")) {
        program = QStandardPaths::findExecutable(QStringLiteral("flatpak"));
        if (m_configuringRemote) {
            arguments = {QStringLiteral("remote-add"), QStringLiteral("--user"),
                         QStringLiteral("--if-not-exists"), QStringLiteral("flathub"),
                         QStringLiteral("https://dl.flathub.org/repo/flathub.flatpakrepo")};
        } else if (m_updating) {
            arguments = {QStringLiteral("update"), QStringLiteral("--user"),
                         QStringLiteral("--noninteractive"), QStringLiteral("--assumeyes"), entry.identifier};
        } else {
            arguments = m_removing
                ? QStringList{QStringLiteral("uninstall"), QStringLiteral("--user"), QStringLiteral("--noninteractive"),
                              QStringLiteral("--assumeyes"), entry.identifier}
                : QStringList{QStringLiteral("install"), QStringLiteral("--user"), QStringLiteral("--noninteractive"),
                              QStringLiteral("--assumeyes"), QStringLiteral("flathub"), entry.identifier};
        }
    } else {
        const QString apt = QStandardPaths::findExecutable(QStringLiteral("apt-get"));
        program = QStandardPaths::findExecutable(QStringLiteral("pkexec"));
        if (apt.isEmpty()) program.clear();
        arguments = {apt, m_removing ? QStringLiteral("remove") : QStringLiteral("install"),
                     QStringLiteral("-y")};
        if (m_updating) arguments.append(QStringLiteral("--only-upgrade"));
        arguments.append(entry.identifier);
    }
    if (program.isEmpty()) {
        setError(tr("The required package service is not installed."));
        m_activeSourceRow = -1;
        m_configuringRemote = false;
        return;
    }
    m_process.start(program, arguments);
}

void StoreModel::finishOperation(int exitCode, QProcess::ExitStatus status)
{
    const int sourceRow = m_activeSourceRow;
    collectOperationOutput();
    const QString output = m_phaseOutput.trimmed();
    if (m_configuringRemote && status == QProcess::NormalExit && exitCode == 0) {
        m_configuringRemote = false;
        startPackageOperation();
        return;
    }
    if (status != QProcess::NormalExit || exitCode != 0) {
        setError(output.isEmpty() ? (m_configuringRemote
            ? tr("The Flathub application source could not be configured.")
            : tr("The package operation failed.")) : output);
    } else if (sourceRow >= 0 && sourceRow < m_entries.size()) {
        m_entries[sourceRow].installed = queryInstalled(m_entries.at(sourceRow));
        const bool verified = m_removing ? !m_entries.at(sourceRow).installed : m_entries.at(sourceRow).installed;
        if (!verified) setError(tr("The package service finished, but the installed state could not be verified. Refresh and inspect the operation details before retrying."));
        else { m_entries[sourceRow].updateAvailable = false; m_lastIdentifier.clear(); }
        emit updatesChanged();
        if (verified) setStatus(m_updating ? tr("Application updated.") : m_removing ? tr("Application removed.") : tr("Application installed."));
    }
    m_activeSourceRow = -1;
    m_configuringRemote = false;
    emit busyChanged();
    if (m_installedOnly || m_updatesOnly) {
        rebuildVisible();
    } else if (!m_visible.isEmpty()) {
        emit dataChanged(index(0), index(m_visible.size() - 1));
    }
}

void StoreModel::collectOperationOutput()
{
    QString output = QString::fromUtf8(m_process.readAllStandardError()
                                      + m_process.readAllStandardOutput());
    if (output.isEmpty()) return;
    output.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    output.remove(QRegularExpression(QStringLiteral("\x1b\\[[0-9;]*[A-Za-z]")));
    m_phaseOutput = (m_phaseOutput + output).right(65536);
    m_operationOutput = (m_operationOutput + output).right(65536);
    auto percentages = QRegularExpression(QStringLiteral("(\\d{1,3})(?:[.,]\\d+)?%")).globalMatch(output);
    while (percentages.hasNext()) m_progress = qBound(0, percentages.next().captured(1).toInt(), 100);
    emit operationOutputChanged();
    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    if (!lines.isEmpty()) setStatus(lines.constLast().trimmed().left(200));
}

void StoreModel::setError(const QString &message)
{
    if (m_errorText == message) return;
    m_errorText = message;
    emit errorTextChanged();
}

void StoreModel::setStatus(const QString &message)
{
    if (m_statusText == message) return;
    m_statusText = message;
    emit statusTextChanged();
}

void StoreModel::setUpdatesOnly(bool value) { if (m_updatesOnly == value) return; m_updatesOnly = value; rebuildVisible(); emit updatesChanged(); }
int StoreModel::updateCount() const { int count = 0; for (const Entry &entry : m_entries) if (entry.updateAvailable) ++count; return count; }
void StoreModel::queryUpdates() {
    const auto connectQuery = [this](QProcess &query, const QString &source) {
        if (query.property("connected").toBool()) return;
        query.setProperty("connected", true);
        connect(&query, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this, &query, source](int code, QProcess::ExitStatus status) {
            if (code != 0 || status != QProcess::NormalExit) return;
            const QString output = QString::fromUtf8(query.readAllStandardOutput());
            QStringList identifiers;
            for (const QString &line : output.split(QLatin1Char('\n'), Qt::SkipEmptyParts))
                identifiers.append(source == QStringLiteral("apt") ? line.section(QLatin1Char('/'), 0, 0).trimmed() : line.trimmed());
            for (Entry &entry : m_entries) if (entry.source == source)
                entry.updateAvailable = entry.installed && identifiers.contains(entry.identifier);
            rebuildVisible(); emit updatesChanged();
        });
    };
    connectQuery(m_aptUpdates, QStringLiteral("apt")); connectQuery(m_flatpakUpdates, QStringLiteral("flatpak"));
    if (m_aptUpdates.state() == QProcess::NotRunning) m_aptUpdates.start(QStringLiteral("apt"), {QStringLiteral("list"), QStringLiteral("--upgradable")});
    if (m_flatpakUpdates.state() == QProcess::NotRunning) m_flatpakUpdates.start(QStringLiteral("flatpak"), {QStringLiteral("remote-ls"), QStringLiteral("--user"), QStringLiteral("--updates"), QStringLiteral("--columns=application")});
}
void StoreModel::updateEntry(int row) {
    if (busy() || row < 0 || row >= m_visible.size()) return;
    const int sourceRow = m_visible.at(row); const Entry &entry = m_entries.at(sourceRow);
    if (!entry.installed || !entry.updateAvailable) return;
    m_activeSourceRow = sourceRow; m_updating = true; m_removing = false;
    m_lastIdentifier = entry.identifier; m_lastRemoving = false; m_lastUpdating = true; m_progress = -1;
    m_operationOutput.clear(); emit operationOutputChanged();
    setError({}); setStatus(tr("Updating %1…").arg(entry.name));
    m_configuringRemote = false;
    startPackageOperation();
}

bool StoreModel::openEntry(int row)
{
    if (row < 0 || row >= m_visible.size() || busy()) return false;
    const Entry &entry = m_entries.at(m_visible.at(row));
    if (!entry.installed) return false;
    m_lastIdentifier.clear();
    bool opened = false;
    if (entry.source == QStringLiteral("flatpak"))
        opened = startDesktopProcess(QStringLiteral("flatpak"), {QStringLiteral("run"), entry.identifier});
    else if (!entry.desktopId.isEmpty()) {
        const QString desktop = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, entry.desktopId);
        if (!desktop.isEmpty()) opened = startDesktopProcess(QStringLiteral("gio"), {QStringLiteral("launch"), desktop});
    }
    if (!opened) setError(tr("The application could not be started. Its desktop launcher may be missing."));
    else { setError({}); setStatus(tr("Starting %1…").arg(entry.name)); }
    return opened;
}

void StoreModel::removeApplication(const QString &identifier)
{
    for (int row = 0; row < m_visible.size(); ++row)
        if (m_entries.at(m_visible.at(row)).identifier == identifier && m_entries.at(m_visible.at(row)).installed) {
            installOrRemove(row); return;
        }
}

void StoreModel::retry()
{
    if (busy() || !canRetry()) return;
    for (int i = 0; i < m_entries.size(); ++i) if (m_entries.at(i).identifier == m_lastIdentifier) {
        m_activeSourceRow = i; m_removing = m_lastRemoving; m_updating = m_lastUpdating;
        m_progress = -1; m_operationOutput.clear(); emit operationOutputChanged();
        setError({}); setStatus(tr("Retrying %1…").arg(m_entries.at(i).name));
        m_configuringRemote = m_entries.at(i).source == QStringLiteral("flatpak") && !m_removing && !m_updating;
        startPackageOperation(); return;
    }
}
