// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>
#include <QProcess>
#include <QFutureWatcher>
#include <QSet>
#include <QVariantMap>

class StoreModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)
    Q_PROPERTY(bool installedOnly READ installedOnly WRITE setInstalledOnly NOTIFY installedOnlyChanged)
    Q_PROPERTY(bool updatesOnly READ updatesOnly WRITE setUpdatesOnly NOTIFY updatesChanged)
    Q_PROPERTY(int updateCount READ updateCount NOTIFY updatesChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)
    Q_PROPERTY(QString operationOutput READ operationOutput NOTIFY operationOutputChanged)
    Q_PROPERTY(int progress READ progress NOTIFY operationOutputChanged)
    Q_PROPERTY(bool canRetry READ canRetry NOTIFY errorTextChanged)
    Q_PROPERTY(QVariantMap selectedApp READ selectedApp NOTIFY selectedAppChanged)
    Q_PROPERTY(QString operationIdentifier READ operationIdentifier NOTIFY operationOutputChanged)
    Q_PROPERTY(QString operationName READ operationName NOTIFY operationOutputChanged)
    Q_PROPERTY(QString phaseText READ phaseText NOTIFY operationOutputChanged)
    Q_PROPERTY(int catalogSize READ catalogSize CONSTANT)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        SummaryRole,
        CategoryRole,
        IconRole,
        SourceRole,
        IdentifierRole,
        InstalledRole,
        EntryBusyRole, UpdateAvailableRole, LaunchableRole, CompatibleRole
    };

    explicit StoreModel(QObject *parent = nullptr);
    ~StoreModel() override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filter() const { return m_filter; }
    void setFilter(const QString &filter);
    QString category() const { return m_category; }
    void setCategory(const QString &category);
    bool installedOnly() const { return m_installedOnly; }
    void setInstalledOnly(bool installedOnly);
    bool updatesOnly() const { return m_updatesOnly; }
    void setUpdatesOnly(bool value);
    int updateCount() const;
    Q_INVOKABLE void updateEntry(int row);
    bool busy() const { return m_process.state() != QProcess::NotRunning || m_refreshing; }
    QString statusText() const { return m_statusText; }
    QString errorText() const { return m_errorText; }
    QString operationOutput() const { return m_operationOutput; }
    int progress() const { return m_progress; }
    bool canRetry() const { return !m_errorText.isEmpty() && !m_lastIdentifier.isEmpty(); }
    Q_INVOKABLE bool openEntry(int row);
    Q_INVOKABLE void removeApplication(const QString &identifier);
    Q_INVOKABLE void retry();
    QVariantMap selectedApp() const;
    QString operationIdentifier() const { return m_operationIdentifier; }
    QString operationName() const { return m_operationName; }
    QString phaseText() const { return m_phaseText; }
    int catalogSize() const { return m_entries.size(); }
    Q_INVOKABLE void selectEntry(int row);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void installSelected();
    Q_INVOKABLE void removeSelected();
    Q_INVOKABLE void updateSelected();
    Q_INVOKABLE bool openSelected();
    Q_INVOKABLE bool openWebsite();

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void installOrRemove(int row);

signals:
    void selectedAppChanged();
    void updatesChanged();
    void filterChanged();
    void categoryChanged();
    void installedOnlyChanged();
    void busyChanged();
    void statusTextChanged();
    void errorTextChanged();
    void operationOutputChanged();

private:
    struct Entry {
        QString name;
        QString summary;
        QString category;
        QString icon;
        QString source;
        QString identifier;
        bool installed = false;
        bool updateAvailable = false;
        QString desktopId;
        QString description;
        QStringList features;
        QString website;
        QStringList architectures;
    };

    void loadCatalog();
    void queryUpdates();
    void rebuildVisible();
    bool queryInstalled(const Entry &entry) const;
    void startPackageOperation();
    void finishOperation(int exitCode, QProcess::ExitStatus status);
    void collectOperationOutput();
    void setError(const QString &message);
    void setStatus(const QString &message);
    int selectedSourceRow() const;
    bool compatible(const Entry &entry) const;
    void beginOperation(int sourceRow, bool removing, bool updating = false);
    bool openApplication(int sourceRow);
    void queryDetails();

    QList<Entry> m_entries;
    QList<int> m_visible;
    QString m_filter;
    QString m_category;
    bool m_installedOnly = false;
    bool m_updatesOnly = false;
    bool m_updating = false;
    QProcess m_aptUpdates;
    QProcess m_flatpakUpdates;
    QString m_statusText;
    QString m_errorText;
    QString m_operationOutput;
    QString m_phaseOutput;
    QProcess m_process;
    int m_activeSourceRow = -1;
    bool m_removing = false;
    bool m_configuringRemote = false;
    int m_progress = -1;
    QString m_lastIdentifier;
    bool m_lastRemoving = false;
    bool m_lastUpdating = false;
    QFutureWatcher<QSet<QString>> m_refresh;
    bool m_refreshing = false;
    QString m_selectedIdentifier;
    QVariantMap m_metadata;
    QProcess *m_details = nullptr;
    QString m_operationIdentifier;
    QString m_operationName;
    QString m_phaseText;
};
