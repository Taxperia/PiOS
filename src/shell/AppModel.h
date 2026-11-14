// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>
#include <QVariantList>
#include <QUrl>

class AppModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)
    Q_PROPERTY(QVariantList pinnedApplications READ pinnedApplications NOTIFY launcherDataChanged)
    Q_PROPERTY(QVariantList recentFiles READ recentFiles NOTIFY launcherDataChanged)
    Q_PROPERTY(QString userName READ userName CONSTANT)
    Q_PROPERTY(QString userHandle READ userHandle CONSTANT)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        IconRole,
        DesktopIdRole,
        CommentRole,
        ExecRole,
        PinnedRole
    };
    Q_ENUM(Role)

    explicit AppModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filter() const { return m_filter; }
    void setFilter(const QString &filter);
    QString category() const { return m_category; }
    void setCategory(const QString &category);
    QVariantList pinnedApplications() const;
    QVariantList recentFiles() const { return m_recentFiles; }
    QString userName() const;
    QString userHandle() const;

    Q_INVOKABLE bool launch(int row);
    Q_INVOKABLE bool addToDesktop(int row);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool launchDesktopId(const QString &desktopId);
    Q_INVOKABLE bool togglePinned(const QString &desktopId);
    Q_INVOKABLE bool openRecentFile(int row);

signals:
    void filterChanged();
    void categoryChanged();
    void launcherDataChanged();

private:
    struct Entry {
        QString name;
        QString icon;
        QString desktopId;
        QString comment;
        QString exec;
        QString sourcePath;
        QStringList categories;
    };

    void rebuildVisibleEntries();
    QList<Entry> discoverApplications() const;
    void readRecentFiles();
    bool launchEntry(const Entry &entry);

    QList<Entry> m_entries;
    QList<Entry> m_visibleEntries;
    QString m_filter;
    QString m_category;
    QStringList m_pinnedIds;
    QVariantList m_recentFiles;
    QList<QUrl> m_recentUrls;
};

