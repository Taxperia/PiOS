// SPDX-License-Identifier: GPL-3.0-or-later
#include "NotificationService.h"

#include <QDBusConnection>
#include <QDateTime>
#include <QTimer>

NotificationService::NotificationService(QObject *parent)
    : QObject(parent)
{
}

bool NotificationService::registerOnSessionBus()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    return bus.registerService(QStringLiteral("org.freedesktop.Notifications"))
        && bus.registerObject(QStringLiteral("/org/freedesktop/Notifications"), this,
                              QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);
}

uint NotificationService::Notify(const QString &appName, uint replacesId, const QString &appIcon,
                                 const QString &summary, const QString &body,
                                 const QStringList &actions, const QVariantMap &hints, int timeout)
{
    Q_UNUSED(hints)
    const uint id = replacesId != 0 && m_actions.contains(replacesId) ? replacesId : m_nextId++;
    m_currentId = id;
    m_appName = appName;
    m_icon = appIcon;
    m_summary = summary;
    m_body = body;
    m_visible = true;
    m_actions.insert(id, actions);
    const uint generation = m_nextGeneration++;
    m_generations.insert(id, generation);
    for (int i = m_history.size() - 1; i >= 0; --i)
        if (m_history.at(i).toMap().value(QStringLiteral("id")).toUInt() == id) m_history.removeAt(i);

    QVariantMap entry;
    entry.insert(QStringLiteral("id"), id);
    entry.insert(QStringLiteral("appName"), appName);
    entry.insert(QStringLiteral("summary"), summary);
    entry.insert(QStringLiteral("body"), body);
    entry.insert(QStringLiteral("icon"), appIcon);
    entry.insert(QStringLiteral("actions"), this->actions());
    entry.insert(QStringLiteral("active"), true);
    entry.insert(QStringLiteral("receivedAt"), QDateTime::currentDateTime().toString(Qt::ISODate));
    m_history.prepend(entry);
    while (m_history.size() > 50) {
        const uint oldest = m_history.last().toMap().value(QStringLiteral("id")).toUInt();
        close(oldest, 1);
        m_history.removeLast();
    }
    emit currentChanged();
    emit historyChanged();

    const int effectiveTimeout = timeout < 0 ? 5000 : timeout;
    if (effectiveTimeout > 0) {
        QTimer::singleShot(effectiveTimeout, this, [this, id, generation] {
            if (m_generations.value(id) == generation) close(id, 1);
        });
    }
    return id;
}

void NotificationService::CloseNotification(uint id)
{
    close(id, 3);
}

QStringList NotificationService::GetCapabilities() const
{
    return {QStringLiteral("body"), QStringLiteral("actions")};
}

void NotificationService::GetServerInformation(QString &name, QString &vendor, QString &version,
                                               QString &specificationVersion) const
{
    name = QStringLiteral("Fidan Notifications");
    vendor = QStringLiteral("Project Fidan");
    version = QStringLiteral("0.1.0");
    specificationVersion = QStringLiteral("1.2");
}

void NotificationService::dismissCurrent()
{
    close(m_currentId, 2);
}

void NotificationService::setHistoryVisible(bool visible)
{
    if (m_historyVisible == visible) {
        return;
    }
    m_historyVisible = visible;
    emit historyVisibleChanged();
}

void NotificationService::toggleHistory()
{
    setHistoryVisible(!m_historyVisible);
}

void NotificationService::clearHistory()
{
    if (m_history.isEmpty()) {
        return;
    }
    const auto ids = m_actions.keys();
    for (uint id : ids) close(id, 2);
    m_history.clear();
    emit historyChanged();
}

QVariantList NotificationService::actions() const
{
    QVariantList result;
    const auto values = m_actions.value(m_currentId);
    for (int i = 0; i + 1 < values.size(); i += 2)
        result.append(QVariantMap{{QStringLiteral("key"), values.at(i)}, {QStringLiteral("label"), values.at(i + 1)}});
    return result;
}

void NotificationService::close(uint id, uint reason)
{
    if (!m_actions.contains(id)) return;
    m_actions.remove(id); m_generations.remove(id);
    for (auto &value : m_history) {
        auto entry = value.toMap();
        if (entry.value(QStringLiteral("id")).toUInt() == id) { entry.insert(QStringLiteral("active"), false); value = entry; }
    }
    if (m_currentId == id) { m_visible = false; emit currentChanged(); }
    emit historyChanged(); emit NotificationClosed(id, reason);
}

void NotificationService::invokeAction(uint id, const QString &key)
{
    const auto values = m_actions.value(id);
    bool valid = false;
    for (int i = 0; i + 1 < values.size(); i += 2) if (values.at(i) == key) valid = true;
    if (!valid) return;
    emit ActionInvoked(id, key);
    close(id, 2);
}

void NotificationService::dismissNotification(uint id) { close(id, 2); }
