// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QUrl>

class ProductInfo final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString channel READ channel CONSTANT)
    Q_PROPERTY(QString supportUrl READ supportUrl CONSTANT)
    Q_PROPERTY(QString accent READ accent CONSTANT)
    Q_PROPERTY(QString accentDark READ accentDark CONSTANT)
    Q_PROPERTY(QUrl wallpaperUrl READ wallpaperUrl CONSTANT)

public:
    explicit ProductInfo(QObject *parent = nullptr);
    explicit ProductInfo(const QString &path, QObject *parent = nullptr);

    QString id() const { return m_id; }
    QString name() const { return m_name; }
    QString version() const { return m_version; }
    QString channel() const { return m_channel; }
    QString supportUrl() const { return m_supportUrl; }
    QString accent() const { return m_accent; }
    QString accentDark() const { return m_accentDark; }
    QUrl wallpaperUrl() const { return m_wallpaperUrl; }

private:
    void load(const QString &path);
    void applyDefaults();

    QString m_id;
    QString m_name;
    QString m_version;
    QString m_channel;
    QString m_supportUrl;
    QString m_accent;
    QString m_accentDark;
    QUrl m_wallpaperUrl;
};

