// SPDX-License-Identifier: GPL-3.0-or-later
#include "ProductInfo.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {
QString productPath()
{
    const QString overridePath = qEnvironmentVariable("FIDAN_PRODUCT_FILE");
    if (!overridePath.isEmpty()) {
        return overridePath;
    }

    const QString installed = QStringLiteral("/usr/share/fidan/product.json");
    if (QFileInfo::exists(installed)) {
        return installed;
    }

    return QString::fromUtf8(FIDAN_SOURCE_DIR) + QStringLiteral("/config/product.json");
}
}

ProductInfo::ProductInfo(QObject *parent)
    : ProductInfo(productPath(), parent)
{
}

ProductInfo::ProductInfo(const QString &path, QObject *parent)
    : QObject(parent)
{
    applyDefaults();
    load(path);
}

void ProductInfo::applyDefaults()
{
    m_id = QStringLiteral("org.fidan.Desktop");
    m_name = QStringLiteral("Project Fidan");
    m_version = QString::fromUtf8(FIDAN_VERSION) + QStringLiteral("-alpha.2");
    m_channel = QStringLiteral("alpha");
    m_supportUrl = QStringLiteral("https://github.com/Taxperia/PiOS/issues");
    m_accent = QStringLiteral("#63DE87");
    m_accentDark = QStringLiteral("#2BA85B");

    const QString installedWallpaper = QStringLiteral("/usr/share/backgrounds/fidan/fidan-mountain-dawn-4k.png");
    const QString sourceWallpaper = QString::fromUtf8(FIDAN_SOURCE_DIR)
        + QStringLiteral("/artwork/wallpapers/fidan-mountain-dawn-4k.png");
    m_wallpaperUrl = QUrl::fromLocalFile(QFileInfo::exists(installedWallpaper)
                                             ? installedWallpaper
                                             : sourceWallpaper);
}

void ProductInfo::load(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return;
    }

    const QJsonObject object = document.object();
    m_id = object.value(QStringLiteral("id")).toString(m_id);
    m_name = object.value(QStringLiteral("name")).toString(m_name);
    m_version = object.value(QStringLiteral("version")).toString(m_version);
    m_channel = object.value(QStringLiteral("channel")).toString(m_channel);
    m_supportUrl = object.value(QStringLiteral("supportUrl")).toString(m_supportUrl);
    m_accent = object.value(QStringLiteral("accent")).toString(m_accent);
    m_accentDark = object.value(QStringLiteral("accentDark")).toString(m_accentDark);

    const QString configuredWallpaper = object.value(QStringLiteral("wallpaper")).toString();
    if (!configuredWallpaper.isEmpty() && QFileInfo::exists(configuredWallpaper)) {
        m_wallpaperUrl = QUrl::fromLocalFile(configuredWallpaper);
    }
}

