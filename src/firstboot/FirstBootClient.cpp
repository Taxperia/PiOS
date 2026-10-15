// SPDX-License-Identifier: GPL-3.0-or-later
#include "FirstBootClient.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>

FirstBootClient::FirstBootClient(QObject *parent)
    : QObject(parent)
{
}

bool FirstBootClient::call(const QString &method, const QVariantList &arguments)
{
    m_busy = true;
    m_error.clear();
    emit busyChanged();
    emit errorChanged();
    QDBusInterface service(QStringLiteral("org.fidan.FirstBoot1"),
                           QStringLiteral("/org/fidan/FirstBoot1"),
                           QStringLiteral("org.fidan.FirstBoot1"),
                           QDBusConnection::systemBus());
    const QDBusMessage reply = service.callWithArgumentList(QDBus::Block, method, arguments);
    const bool result = reply.type() == QDBusMessage::ReplyMessage
        && !reply.arguments().isEmpty() && reply.arguments().first().toBool();
    if (!result) {
        m_error = reply.type() == QDBusMessage::ErrorMessage
            ? reply.errorMessage() : tr("The setup action failed.");
        emit errorChanged();
    }
    m_busy = false;
    emit busyChanged();
    return result;
}

bool FirstBootClient::setLocale(const QString &locale, const QString &keymap, const QString &timezone)
{
    return call(QStringLiteral("SetLocale"), {locale, keymap, timezone});
}
bool FirstBootClient::createUser(const QString &username, const QString &displayName, const QString &password)
{
    return call(QStringLiteral("CreateUser"), {username, displayName, password});
}
bool FirstBootClient::connectWifi(const QString &ssid, const QString &password)
{
    return call(QStringLiteral("ConnectWifi"), {ssid, password});
}
bool FirstBootClient::setPrivacy(bool sshEnabled, bool diagnosticsEnabled)
{
    return call(QStringLiteral("SetPrivacy"), {sshEnabled, diagnosticsEnabled});
}
bool FirstBootClient::complete()
{
    return call(QStringLiteral("Complete"), {});
}

