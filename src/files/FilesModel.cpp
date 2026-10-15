// SPDX-License-Identifier: GPL-3.0-or-later
#include "FilesModel.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QProcess>
#include "../common/DesktopLaunch.h"
#include <QStandardPaths>
#include <QUrl>
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QSettings>
#include <QDateTime>
#include <QtConcurrentRun>
#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

FilesModel::FilesModel(const QString &initialPath, QObject *parent)
    : QAbstractListModel(parent)
{
    auto *clipboard = QGuiApplication::clipboard();
    connect(clipboard, &QClipboard::dataChanged, this, [this] { m_externalClipboard = false; emit clipboardChanged(); });
    connect(&m_clipboardReader, &QFutureWatcher<QStringList>::finished, this, [this] {
        auto snapshot = m_clipboardReader.result();
        m_clipboardUrls.clear();
        if (!snapshot.isEmpty()) m_clipboardCut = snapshot.takeFirst() == QStringLiteral("cut");
        for (const auto &line : snapshot) {
            const QUrl url(line.trimmed());
            if (url.isValid() && !url.scheme().isEmpty()) m_clipboardUrls.append(url);
        }
        m_externalClipboard = true;
        emit clipboardChanged();
    });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { if (!busy()) refresh(); });
    load(initialPath.isEmpty() ? QDir::homePath() : initialPath, true);
}

FilesModel::~FilesModel() { m_transfer.waitForFinished(); m_clipboardReader.waitForFinished(); }

int FilesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visible.size();
}

QVariant FilesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !validRow(index.row()))
        return {};
    const QFileInfo &entry = m_visible.at(index.row());
    switch (role) {
    case NameRole: return entry.fileName();
    case PathRole: return entry.absoluteFilePath();
    case DirectoryRole: return entry.isDir();
    case SizeRole: return entry.isDir() ? QString() : QLocale().formattedDataSize(entry.size());
    case IconRole:
        if (entry.isDir()) {
            const QString name = entry.fileName().toLower();
            if (name == QStringLiteral("documents")) return QStringLiteral("folder-documents");
            if (name == QStringLiteral("downloads")) return QStringLiteral("folder-download");
            if (name == QStringLiteral("pictures")) return QStringLiteral("folder-pictures");
            if (name == QStringLiteral("music")) return QStringLiteral("folder-music");
            if (name == QStringLiteral("videos")) return QStringLiteral("folder-videos");
            return QStringLiteral("folder");
        }
        if (entry.suffix().compare(QStringLiteral("png"), Qt::CaseInsensitive) == 0
            || entry.suffix().compare(QStringLiteral("jpg"), Qt::CaseInsensitive) == 0
            || entry.suffix().compare(QStringLiteral("jpeg"), Qt::CaseInsensitive) == 0)
            return QStringLiteral("image-x-generic");
        return QStringLiteral("text-x-generic");
    default: return {};
    }
}

QHash<int, QByteArray> FilesModel::roleNames() const
{
    return {{NameRole, "name"}, {PathRole, "filePath"}, {DirectoryRole, "directory"},
            {SizeRole, "sizeLabel"}, {IconRole, "iconName"}};
}

void FilesModel::setFilter(const QString &value)
{
    if (m_filter == value)
        return;
    m_filter = value;
    load(m_path, false);
    emit filterChanged();
}

bool FilesModel::load(const QString &requested, bool record)
{
    const QString local = requested.startsWith(QStringLiteral("file:")) ? QUrl(requested).toLocalFile() : requested;
    const QFileInfo target(local);
    if (!target.exists() || !target.isDir()) { setError(tr("This folder cannot be opened.")); return false; }
    const QString clean = QDir::cleanPath(target.isDir() ? target.absoluteFilePath() : target.absolutePath());
    const QDir directory(clean);
    if (!directory.exists() || !directory.isReadable()) {
        setError(tr("This folder cannot be opened."));
        return false;
    }

    if (m_path != clean && !m_filter.isEmpty()) {
        m_filter.clear();
        emit filterChanged();
    }
    auto flags = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System;
    if (m_showHidden) flags |= QDir::Hidden;
    const auto order = m_sortBy == QStringLiteral("size") ? QDir::Size
        : m_sortBy == QStringLiteral("modified") ? QDir::Time : QDir::Name;
    const auto entries = directory.entryInfoList(flags, QDir::DirsFirst | order | QDir::IgnoreCase);
    beginResetModel();
    m_all = entries;
    m_visible.clear();
    for (const QFileInfo &entry : m_all) {
        if (m_filter.isEmpty() || entry.fileName().contains(m_filter, Qt::CaseInsensitive))
            m_visible.append(entry);
    }
    endResetModel();
    setError({});

    if (m_path != clean) {
        if (!m_watcher.directories().isEmpty()) m_watcher.removePaths(m_watcher.directories());
        m_path = clean;
        m_watcher.addPath(m_path);
        emit pathChanged();
        emit clipboardChanged();
    }
    if (record) {
        while (m_history.size() > m_historyIndex + 1)
            m_history.removeLast();
        if (m_history.isEmpty() || m_history.constLast() != clean)
            m_history.append(clean);
        m_historyIndex = m_history.size() - 1;
        emit historyChanged();
    }
    return true;
}

void FilesModel::activate(int row)
{
    if (!validRow(row))
        return;
    const QFileInfo entry = m_visible.at(row);
    if (entry.isDir() && inTrash())
        setError(tr("Restore this folder before opening it."));
    else if (entry.isDir())
        load(entry.absoluteFilePath(), true);
    else if (!openDesktopFile(entry.absoluteFilePath()))
        setError(tr("No application could open this file."));
}

void FilesModel::setPath(const QString &newPath) { load(newPath, true); }

void FilesModel::back()
{
    if (!canGoBack())
        return;
    if (load(m_history.at(m_historyIndex - 1), false)) {
        --m_historyIndex;
        emit historyChanged();
    }
}

void FilesModel::forward()
{
    if (!canGoForward())
        return;
    if (load(m_history.at(m_historyIndex + 1), false)) {
        ++m_historyIndex;
        emit historyChanged();
    }
}

void FilesModel::up()
{
    QDir directory(m_path);
    if (directory.cdUp())
        load(directory.absolutePath(), true);
}

void FilesModel::home() { load(QDir::homePath(), true); }
QString FilesModel::placePath(const QString &place) const
{
    if (place == QStringLiteral("home")) return QDir::homePath();
    if (place == QStringLiteral("projects")) return QDir::home().filePath(QStringLiteral("Projects"));
    if (place == QStringLiteral("trash"))
        return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/Trash/files");
    const QHash<QString, QStandardPaths::StandardLocation> locations {
        {QStringLiteral("desktop"), QStandardPaths::DesktopLocation},
        {QStringLiteral("documents"), QStandardPaths::DocumentsLocation},
        {QStringLiteral("downloads"), QStandardPaths::DownloadLocation},
        {QStringLiteral("pictures"), QStandardPaths::PicturesLocation},
        {QStringLiteral("videos"), QStandardPaths::MoviesLocation},
        {QStringLiteral("music"), QStandardPaths::MusicLocation}
    };
    return locations.contains(place) ? QStandardPaths::writableLocation(locations.value(place)) : QString();
}

bool FilesModel::openPlace(const QString &place)
{
    const QString target = placePath(place);
    if (target.isEmpty()) {
        setError(tr("This location is not available."));
        return false;
    }
    // A fresh user profile may not have its XDG folders yet. Only create
    // known sidebar locations; a mistyped address must still report an error.
    if (!QDir().mkpath(target)) {
        setError(tr("This folder could not be created."));
        return false;
    }
    return load(target, true);
}

void FilesModel::refresh() { load(m_path, false); }

QString FilesModel::entryName(int row) const
{
    return validRow(row) ? m_visible.at(row).fileName() : QString();
}

bool FilesModel::createFolder(const QString &name)
{
    if (busy() || inTrash()) return false;
    const QString cleanName = name.trimmed();
    if (!validName(cleanName)) {
        setError(tr("Enter a valid folder name."));
        return false;
    }
    const QString target = QDir(m_path).filePath(cleanName);
    if (QFileInfo::exists(target) || !QDir(m_path).mkdir(cleanName)) {
        setError(tr("The folder could not be created."));
        return false;
    }
    refresh();
    return true;
}

bool FilesModel::createFile(const QString &name)
{
    if (busy() || inTrash()) return false;
    const QString cleanName = name.trimmed();
    if (!validName(cleanName)) {
        setError(tr("Enter a valid file name."));
        return false;
    }
    QFile file(QDir(m_path).filePath(cleanName));
    if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        setError(tr("The file could not be created."));
        return false;
    }
    file.close();
    refresh();
    return true;
}

bool FilesModel::renameEntry(int row, const QString &newName)
{
    if (busy() || inTrash()) return false;
    if (!validRow(row) || !validName(newName.trimmed())) {
        setError(tr("Enter a valid new name."));
        return false;
    }
    const QFileInfo entry = m_visible.at(row);
    const QString target = QDir(m_path).filePath(newName.trimmed());
    if (QFileInfo::exists(target) || !QDir(m_path).rename(entry.absoluteFilePath(), target)) {
        setError(tr("The item could not be renamed."));
        return false;
    }
    refresh();
    return true;
}

bool FilesModel::moveToTrash(int row)
{
    if (busy() || inTrash()) return false;
    if (!validRow(row))
        return false;
    if (!QFile::moveToTrash(m_visible.at(row).absoluteFilePath())) {
        setError(tr("The item could not be moved to Trash."));
        return false;
    }
    refresh();
    return true;
}

bool FilesModel::deletePermanently(int row)
{
    if (busy()) return false;
    if (!validRow(row))
        return false;
    const QFileInfo entry = m_visible.at(row);
    const bool removed = entry.isDir() && !entry.isSymbolicLink()
        ? QDir(entry.absoluteFilePath()).removeRecursively()
        : QFile::remove(entry.absoluteFilePath());
    if (!removed) {
        setError(tr("The item could not be deleted."));
        return false;
    }
    refresh();
    return true;
}

bool FilesModel::openTerminalHere()
{
    QString terminal = QStandardPaths::findExecutable(QStringLiteral("qterminal"));
    if (!terminal.isEmpty())
        return startDesktopProcess(terminal, {QStringLiteral("--workdir"), m_path}, m_path);
    terminal = QStandardPaths::findExecutable(QStringLiteral("x-terminal-emulator"));
    if (!terminal.isEmpty())
        return startDesktopProcess(terminal, {}, m_path);
    setError(tr("No terminal application is installed."));
    return false;
}

bool FilesModel::validName(const QString &name) const
{
    return !name.isEmpty() && name != QStringLiteral(".") && name != QStringLiteral("..")
        && !name.contains(QLatin1Char('/')) && !name.contains(QLatin1Char('\\'))
        && !name.contains(QChar::Null);
}

bool FilesModel::validRow(int row) const
{
    return row >= 0 && row < m_visible.size();
}

void FilesModel::setError(const QString &message)
{
    if (m_error == message)
        return;
    m_error = message;
    emit errorTextChanged();
}

namespace {
bool present(const QString &path) { const QFileInfo info(path); return info.exists() || info.isSymbolicLink(); }

bool copyTree(const QString &source, const QString &target)
{
    const QFileInfo info(source);
    if (present(target)) return false;
    if (info.isSymbolicLink()) {
#ifdef Q_OS_UNIX
        QByteArray link(1024, Qt::Uninitialized);
        for (;;) {
            const auto length = ::readlink(QFile::encodeName(source).constData(), link.data(), link.size());
            if (length < 0) return false;
            if (length < link.size()) { link.resize(length); break; }
            link.resize(link.size() * 2);
        }
        return ::symlink(link.constData(), QFile::encodeName(target).constData()) == 0;
#else
        return QFile::link(info.symLinkTarget(), target);
#endif
    }
    // Devices, sockets and FIFOs are not ordinary documents. Reading a FIFO
    // could otherwise leave a background transfer waiting indefinitely.
    if (!info.isDir()) return info.isFile() && QFile::copy(source, target);
    if (!info.isReadable()) return false;
    if (!QDir().mkdir(target)) return false;
    const auto children = QDir(source).entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    for (const auto &child : children)
        if (!copyTree(child.absoluteFilePath(), QDir(target).filePath(child.fileName()))) return false;
    if (!QFile::setPermissions(target, info.permissions())) return false;
    return true;
}

bool removeTree(const QString &path)
{
    const QFileInfo info(path);
    return info.isDir() && !info.isSymbolicLink() ? QDir(path).removeRecursively() : QFile::remove(path);
}

QString transferOne(const QString &source, const QString &destination, bool move)
{
    const QFileInfo info(source);
    const QString target = QDir(destination).filePath(info.fileName());
    const QString canonicalSource = info.canonicalFilePath();
    const QString canonicalDestination = QFileInfo(destination).canonicalFilePath();
    if (canonicalDestination.isEmpty() || !QFileInfo(destination).isDir())
        return QObject::tr("The destination folder is unavailable.");
    if (!present(source)) return QObject::tr("The source disappeared: %1").arg(source);
    if (info.isDir() && !info.isSymbolicLink() && (canonicalDestination == canonicalSource
        || canonicalDestination.startsWith(canonicalSource + QLatin1Char('/'))))
        return QObject::tr("A folder cannot be placed inside itself: %1").arg(info.fileName());
    if (present(target)) return QObject::tr("Already exists; nothing was replaced: %1").arg(target);
    if (move && QFile::rename(source, target)) return {};
    if (!copyTree(source, target))
        return QObject::tr("Could not copy %1. A partial copy may remain at %2; the source was kept.").arg(source, target);
    if (move && !removeTree(source))
        return QObject::tr("Copied, but could not remove the original: %1").arg(source);
    return {};
}
}

void FilesModel::setShowHidden(bool value)
{
    if (m_showHidden == value) return;
    m_showHidden = value; refresh(); emit optionsChanged();
}

void FilesModel::setSortBy(const QString &value)
{
    if (m_sortBy == value || !QStringList{QStringLiteral("name"), QStringLiteral("size"), QStringLiteral("modified")}.contains(value)) return;
    m_sortBy = value; refresh(); emit optionsChanged();
}

QList<QUrl> FilesModel::entryUrls(const QVariantList &rows) const
{
    QList<QUrl> urls;
    for (const auto &value : rows) if (validRow(value.toInt())) {
        const auto url = QUrl::fromLocalFile(m_visible.at(value.toInt()).absoluteFilePath());
        if (!urls.contains(url)) urls.append(url);
    }
    return urls;
}

void FilesModel::copyEntries(const QVariantList &rows, bool cut)
{
    if (inTrash() || busy()) return;
    const auto urls = entryUrls(rows);
    if (urls.isEmpty()) return;
    auto *data = new QMimeData;
    data->setUrls(urls);
    data->setData(QStringLiteral("application/x-kde-cutselection"), cut ? "1" : "0");
    QByteArray gnome = cut ? "cut\n" : "copy\n";
    for (const auto &url : urls) gnome += url.toEncoded() + '\n';
    data->setData(QStringLiteral("x-special/gnome-copied-files"), gnome);
    QGuiApplication::clipboard()->setMimeData(data);
}

bool FilesModel::canPaste() const
{
    if (m_externalClipboard) return !m_clipboardUrls.isEmpty() && !inTrash();
    const auto *data = QGuiApplication::clipboard()->mimeData();
    return data && data->hasUrls() && !inTrash();
}

void FilesModel::paste()
{
    if (busy() || !canPaste()) return;
    const auto *data = QGuiApplication::clipboard()->mimeData();
    QStringList sources;
    for (const auto &url : m_externalClipboard ? m_clipboardUrls : data->urls()) {
        if (!url.isLocalFile()) { setError(tr("Only local files can be pasted.")); return; }
        sources.append(url.toLocalFile());
    }
    const bool cut = m_externalClipboard ? m_clipboardCut
        : data->data(QStringLiteral("application/x-kde-cutselection")) == "1"
            || data->data(QStringLiteral("x-special/gnome-copied-files")).startsWith("cut\n");
    transfer(sources, m_path, cut, cut && !m_externalClipboard);
    if (cut && m_externalClipboard) { m_clipboardUrls.clear(); emit clipboardChanged(); }
}

void FilesModel::refreshClipboard()
{
    if (m_clipboardReader.isRunning()) return;
    m_externalClipboard = true; m_clipboardUrls.clear(); emit clipboardChanged();
    // Layer-shell backgrounds cannot take keyboard focus. wl-paste uses the
    // compositor's data-control protocol to read the file selection there.
    m_clipboardReader.setFuture(QtConcurrent::run([] {
        const auto read = [](const QString &type) {
            QProcess process;
            process.start(QStringLiteral("wl-paste"), {QStringLiteral("--no-newline"), QStringLiteral("--type"), type});
            if (!process.waitForFinished(1000)) { process.kill(); process.waitForFinished(); return QString(); }
            return process.exitCode() == 0 ? QString::fromUtf8(process.readAllStandardOutput()) : QString();
        };
        const QString gnome = read(QStringLiteral("x-special/gnome-copied-files"));
        if (gnome.startsWith(QStringLiteral("copy\n")) || gnome.startsWith(QStringLiteral("cut\n")))
            return gnome.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        QStringList result{read(QStringLiteral("application/x-kde-cutselection")).trimmed() == QStringLiteral("1")
            ? QStringLiteral("cut") : QStringLiteral("copy")};
        for (const auto &line : read(QStringLiteral("text/uri-list")).split(QLatin1Char('\n'), Qt::SkipEmptyParts))
            if (!line.startsWith(QLatin1Char('#'))) result.append(line);
        return result;
    }));
}

void FilesModel::dropUrls(const QList<QUrl> &urls, const QString &destination, bool move)
{
    if (busy() || inTrash()) return;
    QStringList sources;
    for (const auto &url : urls) {
        if (!url.isLocalFile()) { setError(tr("Only local files can be dropped here.")); return; }
        sources.append(url.toLocalFile());
    }
    transfer(sources, destination.isEmpty() ? m_path : destination, move, false);
}

void FilesModel::transfer(const QStringList &sources, const QString &destination, bool move, bool clipboard)
{
    if (sources.isEmpty() || busy() || inTrash()) return;
    runOperation([sources, destination, move] {
        QStringList errors;
        for (const auto &source : sources) {
            const QString error = transferOne(source, destination, move);
            if (!error.isEmpty()) errors.append(error);
        }
        return errors.join(QLatin1Char('\n'));
    }, move ? tr("Moving items…") : tr("Copying items…"), clipboard);
}

void FilesModel::runOperation(const std::function<QString()> &operation, const QString &status, bool clearClipboard)
{
    if (busy()) return;
    setError({}); m_operationStatus = status;
    const auto originalUrls = QGuiApplication::clipboard()->mimeData()
        ? QGuiApplication::clipboard()->mimeData()->urls() : QList<QUrl>{};
    disconnect(&m_transfer, nullptr, this, nullptr);
    connect(&m_transfer, &QFutureWatcher<QString>::finished, this, [this, clearClipboard, originalUrls] {
        const QString error = m_transfer.result();
        if (clearClipboard && error.isEmpty() && QGuiApplication::clipboard()->mimeData()
            && QGuiApplication::clipboard()->mimeData()->urls() == originalUrls)
            QGuiApplication::clipboard()->clear();
        m_busy = false;
        refresh(); setError(error);
        m_operationStatus = error.isEmpty() ? tr("Operation completed.") : tr("Some items could not be processed.");
        emit busyChanged();
    });
    m_busy = true;
    m_transfer.setFuture(QtConcurrent::run(operation));
    emit busyChanged();
}

void FilesModel::trashEntries(const QVariantList &rows, bool permanently)
{
    if (busy() || (inTrash() && !permanently)) return;
    QStringList paths;
    for (const auto &url : entryUrls(rows)) paths.append(url.toLocalFile());
    const bool trashView = inTrash();
    runOperation([paths, permanently, trashView] {
        QStringList errors;
        for (const auto &path : paths) {
            const bool ok = permanently ? removeTree(path) : QFile::moveToTrash(path);
            if (!ok) errors.append(QObject::tr("Could not remove: %1").arg(path));
            else if (trashView) QFile::remove(QFileInfo(path).dir().absolutePath() + QStringLiteral("/../info/") + QFileInfo(path).fileName() + QStringLiteral(".trashinfo"));
        }
        return errors.join(QLatin1Char('\n'));
    }, permanently ? tr("Deleting items…") : tr("Moving to Trash…"));
}

void FilesModel::restoreEntries(const QVariantList &rows)
{
    if (busy() || !inTrash()) return;
    QStringList paths;
    for (const auto &url : entryUrls(rows)) paths.append(url.toLocalFile());
    runOperation([paths] {
        QStringList errors;
        for (const auto &path : paths) {
            const QString metadata = QFileInfo(path).dir().absolutePath() + QStringLiteral("/../info/") + QFileInfo(path).fileName() + QStringLiteral(".trashinfo");
            QFile info(metadata);
            QString original;
            if (info.open(QIODevice::ReadOnly)) {
                for (const auto &line : info.readAll().split('\n')) if (line.startsWith("Path="))
                    original = QUrl::fromPercentEncoding(line.mid(5));
            }
            if (!QDir::isAbsolutePath(original) || present(original) || !QFileInfo(original).dir().exists()) {
                errors.append(QObject::tr("Cannot restore %1: its original folder is missing or its name is already in use.").arg(QFileInfo(path).fileName()));
                continue;
            }
            // Trash filenames can have a suffix; restore the original name.
            if (!QFile::rename(path, original)) errors.append(QObject::tr("Could not restore: %1").arg(original));
            else QFile::remove(metadata);
        }
        return errors.join(QLatin1Char('\n'));
    }, tr("Restoring items…"));
}

QVariantMap FilesModel::properties(int row) const
{
    if (!validRow(row)) return {};
    const QFileInfo info = m_visible.at(row);
    return {{QStringLiteral("name"), info.fileName()}, {QStringLiteral("path"), info.absoluteFilePath()},
        {QStringLiteral("type"), info.isSymbolicLink() ? tr("Symbolic link") : info.isDir() ? tr("Folder") : tr("File")},
        {QStringLiteral("size"), info.isDir() ? tr("Folder") : QLocale().formattedDataSize(info.size())},
        {QStringLiteral("modified"), QLocale().toString(info.lastModified(), QLocale::ShortFormat)},
        {QStringLiteral("owner"), info.owner()}, {QStringLiteral("readable"), info.isReadable()},
        {QStringLiteral("writable"), info.isWritable()}, {QStringLiteral("linkTarget"), info.isSymbolicLink() ? info.symLinkTarget() : QString()}};
}
