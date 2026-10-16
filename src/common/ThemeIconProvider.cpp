// SPDX-License-Identifier: GPL-3.0-or-later
#include "ThemeIconProvider.h"
#include "IconLookup.h"

#include <QIcon>
#include <QPainter>
#include <QUrl>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

ThemeIconProvider::ThemeIconProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
    auto paths = QIcon::themeSearchPaths();
    for (const auto &directory : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
        const QString path = directory + QStringLiteral("/icons");
        if (!paths.contains(path)) paths.append(path);
    }
    if (!paths.contains(QStringLiteral("/usr/share/icons"))) paths.append(QStringLiteral("/usr/share/icons"));
    QIcon::setThemeSearchPaths(paths);
    QIcon::setThemeName(QStringLiteral("Papirus-Dark"));
    QIcon::setFallbackThemeName(QStringLiteral("hicolor"));
}

QPixmap ThemeIconProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QString iconName = QUrl::fromPercentEncoding(id.section(QLatin1Char('?'), 0, 0).toUtf8());
    const bool dark = id.contains(QStringLiteral("theme=dark")) || (!id.contains(QStringLiteral("theme=light"))
        && QSettings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
            + QStringLiteral("/fidan/shell.conf"), QSettings::IniFormat).value(QStringLiteral("appearance/darkTheme"), true).toBool());
    QIcon::setThemeName(dark ? QStringLiteral("Papirus-Dark") : QStringLiteral("Papirus"));
    if (iconName == QStringLiteral("folder") || iconName.startsWith(QStringLiteral("folder-"))) {
        const QSize target = requestedSize.isValid() ? requestedSize : QSize(64, 64);
        QPixmap folder(target); folder.fill(Qt::transparent);
        QPainter painter(&folder); painter.setRenderHint(QPainter::Antialiasing);
        painter.scale(target.width() / 64.0, target.height() / 64.0);
        painter.setPen(Qt::NoPen); painter.setBrush(QColor(QStringLiteral("#4CBF78")));
        painter.drawRoundedRect(QRectF(7, 12, 24, 16), 3, 3);
        painter.drawRoundedRect(QRectF(7, 19, 50, 34), 4, 4);
        painter.setBrush(QColor(QStringLiteral("#8CDEAA")));
        painter.drawRoundedRect(QRectF(7, 25, 50, 29), 4, 4);
        painter.setBrush(QColor(QStringLiteral("#245A3A")));
        if (iconName.contains(QStringLiteral("videos"))) {
            QPolygonF triangle; triangle << QPointF(28,33) << QPointF(28,46) << QPointF(40,39.5);
            painter.drawPolygon(triangle);
        } else if (iconName.contains(QStringLiteral("music"))) {
            painter.drawEllipse(QRectF(24,40,9,7)); painter.drawRect(QRectF(31,31,3,13)); painter.drawRect(QRectF(32,31,8,3));
        } else {
            painter.drawRoundedRect(QRectF(25,34,14,10), 1.5, 1.5);
            painter.setPen(QPen(QColor(QStringLiteral("#D4F4DD")), 2));
            painter.drawLine(QPointF(28,38), QPointF(36,38));
        }
        if (size) *size = target;
        return folder;
    }
    QIcon icon = lookupDesktopIcon(id.section(QLatin1Char('?'), 0, 0));
    if (icon.isNull()) icon = lookupDesktopIcon(iconName);
    if (icon.isNull()) icon = lookupDesktopIcon(QStringLiteral("application-x-executable"));
    const QSize target = requestedSize.isValid() ? requestedSize : QSize(64, 64);
    QPixmap pixmap = icon.pixmap(target);
    if (pixmap.isNull()) {
        pixmap = QPixmap(target);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(QStringLiteral("#315F46")));
        painter.drawRoundedRect(pixmap.rect().adjusted(1, 1, -1, -1), target.width() / 5.0,
                                target.height() / 5.0);
        painter.setBrush(QColor(QStringLiteral("#EAF7EF")));
        const int unit = qMax(2, qMin(target.width(), target.height()) / 6);
        const int gap = qMax(2, unit / 2);
        const int left = (target.width() - unit * 2 - gap) / 2;
        const int top = (target.height() - unit * 2 - gap) / 2;
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                painter.drawRoundedRect(QRect(left + x * (unit + gap), top + y * (unit + gap), unit, unit),
                                        unit / 4.0, unit / 4.0);
            }
        }
    }
    if (iconName.endsWith(QStringLiteral("-symbolic")) && !pixmap.isNull()) {
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), QColor(dark ? "#F2F7F4" : "#18261F"));
    }
    if (size) *size = pixmap.size();
    return pixmap;
}
