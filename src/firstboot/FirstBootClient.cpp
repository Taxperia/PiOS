// SPDX-License-Identifier: GPL-3.0-or-later
#include "FirstBootClient.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusArgument>
#include <QCoreApplication>

namespace {
QDBusInterface *service(const QDBusConnection &bus, QObject *parent) {
    auto *value = new QDBusInterface("org.fidan.FirstBoot1", "/org/fidan/FirstBoot1", "org.fidan.FirstBoot1", bus, parent);
    value->setTimeout(180000);
    return value;
}
}
FirstBootClient::FirstBootClient(QObject *parent) : FirstBootClient(QDBusConnection::systemBus(), parent) {}
FirstBootClient::FirstBootClient(const QDBusConnection &bus, QObject *parent) : QObject(parent), m_bus(bus) {
    m_poll.setInterval(1000); connect(&m_poll, &QTimer::timeout, this, &FirstBootClient::refresh);
}
bool FirstBootClient::call(const QString &method, const QVariantList &arguments) {
    if (busy()) return false;
    m_busy = true; m_error.clear(); emit busyChanged(); emit errorChanged();
    auto *endpoint = service(m_bus, this);
    auto *watcher = new QDBusPendingCallWatcher(endpoint->asyncCallWithArgumentList(method, arguments), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, endpoint, watcher, method] {
        QDBusPendingReply<bool> reply = *watcher;
        const bool ok = !reply.isError() && reply.value();
        if (!ok) m_error = reply.isError() ? QCoreApplication::translate("FirstBootService",reply.error().message().toUtf8().constData()) : tr("The setup action failed. Your progress is saved; retry this step.");
        m_busy = false; emit busyChanged(); emit errorChanged();
        watcher->deleteLater(); endpoint->deleteLater();
        refresh(); emit operationFinished(method, ok);
    });
    return true;
}
void FirstBootClient::refresh() {
    if (m_refreshing) return;
    m_refreshing = true;
    auto *endpoint = service(m_bus, this);
    auto *watcher = new QDBusPendingCallWatcher(endpoint->asyncCall("State"), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, endpoint, watcher] {
        QDBusPendingReply<QVariantMap> reply = *watcher;
        m_refreshing = false;
        if (!reply.isError()) {
            m_state = reply.value();
            if (m_state.contains("error")) { m_error = m_state.value("error").toString(); emit errorChanged(); }
            if (m_state.value("updateBusy").toBool()) m_poll.start(); else m_poll.stop();
            emit stateChanged(); emit busyChanged();
        } else { m_error = reply.error().message(); emit errorChanged(); }
        watcher->deleteLater(); endpoint->deleteLater();
    });
}
void FirstBootClient::scanWifi() {
    if (busy()) return;
    m_busy = true; m_error.clear(); emit busyChanged(); emit errorChanged();
    auto *endpoint = service(m_bus, this);
    auto *watcher = new QDBusPendingCallWatcher(endpoint->asyncCall("ScanWifi"), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, endpoint, watcher] {
        QDBusPendingReply<QStringList> reply = *watcher;
        if (reply.isError()) m_error = reply.error().message(); else m_networks = reply.value();
        m_busy = false; emit networksChanged(); emit busyChanged(); emit errorChanged();
        watcher->deleteLater(); endpoint->deleteLater();
    });
}
bool FirstBootClient::setLocale(const QString &locale, const QString &keymap, const QString &timezone) { return call("SetLocale", {locale, keymap, timezone}); }
bool FirstBootClient::createUser(const QString &username, const QString &displayName, const QString &password) { return call("CreateUser", {username, displayName, password}); }
bool FirstBootClient::connectWifi(const QString &ssid, const QString &password) { return call("ConnectWifi", {ssid, password}); }
bool FirstBootClient::setPrivacy(bool ssh, bool diagnostics, bool remote) { return call("SetPrivacy", {ssh, diagnostics, remote}); }
bool FirstBootClient::complete() { return call("Complete", {}); }
bool FirstBootClient::setDesktopPreferences(const QVariantMap &preferences) { return call("SetDesktopPreferences", {preferences}); }
bool FirstBootClient::skipNetwork() { return call("SkipNetwork", {}); }
bool FirstBootClient::startUpdate(const QString &action) { return call("StartUpdate", {action}); }
bool FirstBootClient::skipUpdates() { return call("SkipUpdates", {}); }
void FirstBootClient::setLanguage(const QString &locale) { emit languageChanged(locale); }
