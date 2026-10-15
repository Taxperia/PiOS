// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QUrl>
#include <QSize>
#include <QFutureWatcher>
#include <QImage>

class PhotoModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url NOTIFY currentChanged)
    Q_PROPERTY(QString name READ name NOTIFY currentChanged)
    Q_PROPERTY(QString details READ details NOTIFY currentChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorChanged)
    Q_PROPERTY(int pixelWidth READ pixelWidth NOTIFY currentChanged)
    Q_PROPERTY(int pixelHeight READ pixelHeight NOTIFY currentChanged)
    Q_PROPERTY(int position READ position NOTIFY currentChanged)
    Q_PROPERTY(int count READ count NOTIFY currentChanged)
    Q_PROPERTY(int rotation READ rotation NOTIFY rotationChanged)
    Q_PROPERTY(bool copying READ copying NOTIFY copyingChanged)
public:
    explicit PhotoModel(QObject *parent = nullptr);
    ~PhotoModel() override;
    QUrl url() const { return m_url; }
    QString name() const { return m_name; }
    QString details() const { return m_details; }
    QString errorText() const { return m_error; }
    int pixelWidth() const { return m_size.width(); }
    int pixelHeight() const { return m_size.height(); }
    int position() const { return m_index < 0 ? 0 : m_index + 1; }
    int count() const { return m_files.size(); }
    int rotation() const { return m_rotation; }
    bool copying() const { return m_copying; }
    Q_INVOKABLE bool open(const QUrl &url);
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void rotate(int direction);
    Q_INVOKABLE void copyImage();
    Q_INVOKABLE bool useAsWallpaper();
    Q_INVOKABLE void decodeFailed();
    static QStringList supportedExtensions();
signals:
    void currentChanged();
    void rotationChanged();
    void errorChanged();
    void copyingChanged();
    void message(const QString &text);
private:
    bool load(const QString &path);
    void setError(const QString &text);
    QUrl m_url;
    QString m_name, m_details, m_error;
    QSize m_size;
    QStringList m_files;
    int m_index = -1, m_rotation = 0;
    QFutureWatcher<QImage> m_copy;
    bool m_copying = false;
};
