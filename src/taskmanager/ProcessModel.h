// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>
#include <QHash>

class ProcessModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(double totalCpu READ totalCpu NOTIFY totalsChanged)
    Q_PROPERTY(double totalMemory READ totalMemory NOTIFY totalsChanged)
    Q_PROPERTY(int processCount READ processCount NOTIFY totalsChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

public:
    enum Role {
        PidRole = Qt::UserRole + 1,
        NameRole,
        UserRole,
        CpuRole,
        MemoryRole,
        MemoryBytesRole,
        StateRole,
        CommandRole
    };

    explicit ProcessModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filter() const { return m_filter; }
    void setFilter(const QString &filter);
    double totalCpu() const { return m_totalCpu; }
    double totalMemory() const { return m_totalMemory; }
    int processCount() const { return m_entries.size(); }
    QString errorText() const { return m_errorText; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool terminateProcess(int pid, bool force = false);

signals:
    void filterChanged();
    void totalsChanged();
    void errorTextChanged();

private:
    struct Entry {
        int pid = 0;
        QString name;
        QString user;
        double cpu = 0;
        double memory = 0;
        quint64 memoryBytes = 0;
        QString state;
        QString command;
        quint64 processTicks = 0;
    };

    static QPair<quint64, quint64> systemTicks();
    static quint64 totalMemoryBytes();
    static bool readProcess(int pid, Entry &entry);
    void rebuildVisible();
    void setError(const QString &error);

    QList<Entry> m_entries;
    QList<int> m_visible;
    QHash<int, quint64> m_previousProcessTicks;
    quint64 m_previousSystemTicks = 0;
    quint64 m_previousIdleTicks = 0;
    QString m_filter;
    QString m_errorText;
    double m_totalCpu = 0;
    double m_totalMemory = 0;
};
