// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QMimeDatabase>
#include <QUrl>

// LayerShellQt changes the shell process environment. Never pass its surface
// role to a desktop application: applications must use xdg-toplevel windows.
inline bool startDesktopProcess(const QString &program, const QStringList &arguments = {},
                                const QString &workingDirectory = {})
{
    QString executable = QStandardPaths::findExecutable(program);
    if (executable.isEmpty() && program.startsWith(QStringLiteral("fidan-"))) {
        const QDir binaries(QCoreApplication::applicationDirPath());
        const QString component = program == QStringLiteral("fidan-task-manager") ? QStringLiteral("taskmanager") : program.mid(6);
        const QString sibling = binaries.filePath(QStringLiteral("../") + component + QLatin1Char('/') + program);
        if (QFileInfo(sibling).isExecutable()) executable = sibling;
    }
    if (executable.isEmpty()) executable = program;
    QProcess process;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("QT_WAYLAND_SHELL_INTEGRATION"), QStringLiteral("xdg-shell"));
    environment.remove(QStringLiteral("QT_WAYLAND_DISABLE_WINDOWDECORATION"));
    process.setProcessEnvironment(environment);
    process.setProgram(executable);
    process.setArguments(arguments);
    if (!workingDirectory.isEmpty()) process.setWorkingDirectory(workingDirectory);
    return process.startDetached();
}

// Text documents have a shipped editor even when the host has no MIME launcher.
// Keep the path as a single argument, including spaces and non-ASCII names.
inline bool openDesktopFile(const QString &path)
{
    const QFileInfo file(path);
    if (!file.exists() || !file.isFile() || !file.isReadable()) return false;
    const QString absolutePath = file.absoluteFilePath();
    const auto mime = QMimeDatabase().mimeTypeForFile(absolutePath, QMimeDatabase::MatchExtension);
    // Honor the user's MIME choice. The shipped editor is only a fallback
    // when the profile has no registered text handler.
    QProcess query;
    query.start(QStringLiteral("xdg-mime"), {QStringLiteral("query"), QStringLiteral("default"), mime.name()});
    const bool configured = query.waitForFinished(500) && !query.readAllStandardOutput().trimmed().isEmpty();
    if (configured) return startDesktopProcess(QStringLiteral("xdg-open"), {absolutePath});
    if (!configured && (mime.inherits(QStringLiteral("text/plain")) || mime.name().startsWith(QStringLiteral("text/")))
        && startDesktopProcess(QStringLiteral("featherpad"), {absolutePath})) return true;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(absolutePath));
}
