// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>
#include <QFileSystemWatcher>

class DesktopModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
public:
    enum Role { NameRole = Qt::UserRole + 1, IconRole, PathRole, DirectoryRole, DesktopFileRole };
    explicit DesktopModel(QObject *parent = nullptr);
    explicit DesktopModel(const QString &directory, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool open(int row);
    QString sortMode() const { return m_sortMode; }
    void setSortMode(const QString &mode);

signals:
    void sortModeChanged();

private:
    struct Entry { QString name; QString icon; QString path; bool directory = false; bool desktopFile = false; qint64 modified = 0; QString suffix; };
    QString desktopPath() const;
    QList<Entry> m_entries;
    QFileSystemWatcher m_watcher;
    QString m_directory;
    QString m_sortMode = QStringLiteral("Name");
};
