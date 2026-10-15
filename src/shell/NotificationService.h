// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QHash>

class NotificationService final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")
    Q_PROPERTY(bool visible READ visible NOTIFY currentChanged)
    Q_PROPERTY(QString appName READ appName NOTIFY currentChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY currentChanged)
    Q_PROPERTY(QString body READ body NOTIFY currentChanged)
    Q_PROPERTY(QString icon READ icon NOTIFY currentChanged)
    Q_PROPERTY(QVariantList actions READ actions NOTIFY currentChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)
    Q_PROPERTY(bool historyVisible READ historyVisible WRITE setHistoryVisible NOTIFY historyVisibleChanged)

public:
    explicit NotificationService(QObject *parent = nullptr);
    bool registerOnSessionBus();
    bool visible() const { return m_visible; }
    QString appName() const { return m_appName; }
    QString summary() const { return m_summary; }
    QString body() const { return m_body; }
    QString icon() const { return m_icon; }
    QVariantList actions() const;
    QVariantList history() const { return m_history; }
    bool historyVisible() const { return m_historyVisible; }
    void setHistoryVisible(bool visible);

public slots:
    uint Notify(const QString &appName, uint replacesId, const QString &appIcon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int timeout);
    void CloseNotification(uint id);
    QStringList GetCapabilities() const;
    void GetServerInformation(QString &name, QString &vendor, QString &version,
                              QString &specificationVersion) const;
    void dismissCurrent();
    void toggleHistory();
    void clearHistory();
    void invokeAction(uint id, const QString &key);
    void invokeCurrentAction(const QString &key) { invokeAction(m_currentId, key); }
    void dismissNotification(uint id);

signals:
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &actionKey);
    void currentChanged();
    void historyChanged();
    void historyVisibleChanged();

private:
    uint m_nextId = 1;
    uint m_currentId = 0;
    bool m_visible = false;
    QString m_appName;
    QString m_summary;
    QString m_body;
    QString m_icon;
    QVariantList m_history;
    bool m_historyVisible = false;
    void close(uint id, uint reason);
    QHash<uint, QStringList> m_actions;
    QHash<uint, uint> m_generations;
    uint m_nextGeneration = 1;
};
