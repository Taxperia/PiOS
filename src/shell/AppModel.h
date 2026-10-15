// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>

class AppModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        IconRole,
        DesktopIdRole,
        CommentRole,
        ExecRole
    };
    Q_ENUM(Role)

    explicit AppModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filter() const { return m_filter; }
    void setFilter(const QString &filter);

    Q_INVOKABLE bool launch(int row);
    Q_INVOKABLE bool addToDesktop(int row);
    Q_INVOKABLE void refresh();

signals:
    void filterChanged();

private:
    struct Entry {
        QString name;
        QString icon;
        QString desktopId;
        QString comment;
        QString exec;
        QString sourcePath;
    };

    void rebuildVisibleEntries();
    QList<Entry> discoverApplications() const;

    QList<Entry> m_entries;
    QList<Entry> m_visibleEntries;
    QString m_filter;
};

