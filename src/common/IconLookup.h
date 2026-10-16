// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHash>
#include <QIcon>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

// Prefer PiOS artwork directly: a running process can have an old or missing
// icon-theme cache after a package adds new icons.
inline QIcon lookupDesktopIcon(QString name)
{
    name=name.trimmed();
    if(QDir::isAbsolutePath(name)) return QFileInfo(name).isFile()?QIcon(name):QIcon();
    const QUrl url(name);
    if(url.isLocalFile()) name=url.toLocalFile();
    if(QDir::isAbsolutePath(name)) return QFileInfo(name).isFile()?QIcon(name):QIcon();
    static const QHash<QString,QString> aliases {
        {"fidan-shell","org.fidan.Shell"},{"fidan-files","org.fidan.Files"},
        {"fidan-store","org.fidan.Store"},{"fidan-settings","org.fidan.Settings"},
        {"fidan-task-manager","org.fidan.TaskManager"},{"fidan-photos","org.fidan.Photos"},
        {"fidan-firstboot","org.fidan.FirstBoot"},{"fidan-calculator","org.fidan.Calculator"},
        {"fidan-clock","org.fidan.Clock"},{"fidan-calendar","org.fidan.Calendar"},
        {"fidan-pdf","org.fidan.Pdf"},{"file","application-octet-stream"},
        {"text-plain","text-x-generic"},{"text-plain-symbolic","text-x-generic"}
    };
    if(name.endsWith(".desktop")) name.chop(8);
    name=aliases.value(name,name);
    if(!name.isEmpty() && !name.contains('/') && !name.contains('\\')) {
        for(const auto &directory:{QString(":/fidan/icons/"),QString(":/fidan/icons/mimetypes/")}) {
            const QString path=directory+name+".svg";
            if(QFileInfo::exists(path)) return QIcon(path);
        }
        // Also work in development binaries without embedded artwork and when
        // a theme inherits hicolor incorrectly. Flatpak export roots are XDG data.
        for(const auto &root:QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation))
            for(const auto &context:{QString("apps"),QString("mimetypes")}) {
                const QString path=root+"/icons/hicolor/scalable/"+context+"/"+name+".svg";
                if(QFileInfo::exists(path)) return QIcon(path);
            }
    }
    QIcon icon=QIcon::fromTheme(name);
    if(!icon.isNull()) return icon;
    if(name.contains('/') || name.contains('\\')) return {};
    for(const auto &size:{"48x48","32x32","24x24","22x22","16x16","scalable"})
        for(const auto &context:{"apps","mimetypes","places","actions","categories","devices","status"}) {
            const QString path=QString("/usr/share/icons/Papirus/%1/%2/%3.svg").arg(size,context,name);
            if(QFileInfo::exists(path)) return QIcon(path);
        }
    return {};
}

inline void setPiOSApplicationIdentity(QGuiApplication &app,const QString &desktopId)
{
    app.setDesktopFileName(desktopId);
    app.setWindowIcon(lookupDesktopIcon(desktopId));
}
