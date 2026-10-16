// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QFileInfo>
#include <QMimeDatabase>
#include <QSettings>

inline QString fileIconName(const QFileInfo &file)
{
    if(file.isDir()) return QStringLiteral("folder");
    const QString suffix=file.suffix().toLower();
    if(suffix=="desktop") {
        QSettings shortcut(file.absoluteFilePath(),QSettings::IniFormat);
        shortcut.beginGroup("Desktop Entry");
        const QString icon=shortcut.value("Icon").toString().trimmed();
        return icon.isEmpty()?QStringLiteral("application-x-executable"):icon;
    }
    const auto mime=QMimeDatabase().mimeTypeForFile(file,QMimeDatabase::MatchExtension);
    const QString name=mime.name();
    if(mime.inherits("application/pdf")) return QStringLiteral("application-pdf");
    if(name.startsWith("image/")) return QStringLiteral("image-x-generic");
    if(name.startsWith("audio/")) return QStringLiteral("audio-x-generic");
    if(name.startsWith("video/")) return QStringLiteral("video-x-generic");
    if(QStringList{"zip","gz","xz","bz2","tar","7z","rar","zst","tgz"}.contains(suffix)) return QStringLiteral("package-x-generic");
    if(suffix=="deb" || suffix=="rpm" || suffix=="appimage") return QStringLiteral("application-x-executable");
    if(QStringList{"cpp","c","h","hpp","py","js","ts","qml","json","html","css","sh","xml","yaml","yml"}.contains(suffix)) return QStringLiteral("text-x-script");
    if(name.startsWith("text/") || mime.inherits("text/plain")) return QStringLiteral("text-x-generic");
    if(name.contains("spreadsheet") || name.contains("excel")) return QStringLiteral("x-office-spreadsheet");
    if(name.contains("presentation") || name.contains("powerpoint")) return QStringLiteral("x-office-presentation");
    if(name.contains("document") || name.contains("word")) return QStringLiteral("x-office-document");
    return mime.iconName().isEmpty()?QStringLiteral("application-octet-stream"):mime.iconName();
}
