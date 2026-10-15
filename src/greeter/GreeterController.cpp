// SPDX-License-Identifier: GPL-3.0-or-later
#include "GreeterController.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QtEndian>

GreeterController::GreeterController(QObject *parent)
    : QObject(parent)
{
    connect(&m_socket, &QLocalSocket::readyRead, this, &GreeterController::readResponse);
    connect(&m_socket, &QLocalSocket::errorOccurred, this, &GreeterController::socketFailed);
    m_socket.connectToServer(qEnvironmentVariable("GREETD_SOCK"));
}

void GreeterController::beginLogin(const QString &username)
{
    if (username.trimmed().isEmpty()) {
        setError(tr("Enter a username."));
        return;
    }
    m_busy = true;
    m_error.clear();
    emit stateChanged();
    send({{QStringLiteral("type"), QStringLiteral("create_session")},
          {QStringLiteral("username"), username.trimmed()}});
}

void GreeterController::answer(const QString &response)
{
    send({{QStringLiteral("type"), QStringLiteral("post_auth_message_response")},
          {QStringLiteral("response"), response}});
}

void GreeterController::cancel()
{
    send({{QStringLiteral("type"), QStringLiteral("cancel_session")}});
    m_busy = false;
    m_prompt.clear();
    emit stateChanged();
}

void GreeterController::powerOff()
{
    QProcess::startDetached(QStringLiteral("systemctl"), {QStringLiteral("poweroff")});
}

void GreeterController::reboot()
{
    QProcess::startDetached(QStringLiteral("systemctl"), {QStringLiteral("reboot")});
}

void GreeterController::send(const QJsonObject &message)
{
    if (m_socket.state() != QLocalSocket::ConnectedState) {
        setError(tr("The login service is unavailable."));
        return;
    }
    const QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    QByteArray frame;
    const quint32 size = qToLittleEndian(static_cast<quint32>(payload.size()));
    frame.append(reinterpret_cast<const char *>(&size), sizeof(size));
    frame.append(payload);
    m_socket.write(frame);
    m_socket.flush();
}

void GreeterController::readResponse()
{
    m_buffer.append(m_socket.readAll());
    while (m_buffer.size() >= 4) {
        const quint32 size = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(m_buffer.constData()));
        if (m_buffer.size() < 4 + static_cast<int>(size)) {
            return;
        }
        const QJsonObject response = QJsonDocument::fromJson(m_buffer.mid(4, size)).object();
        m_buffer.remove(0, 4 + size);
        const QString type = response.value(QStringLiteral("type")).toString();
        if (type == QStringLiteral("auth_message")) {
            m_prompt = response.value(QStringLiteral("auth_message")).toString(tr("Password"));
            m_busy = false;
            emit stateChanged();
        } else if (type == QStringLiteral("success")) {
            emit authenticated();
            send({{QStringLiteral("type"), QStringLiteral("start_session")},
                  {QStringLiteral("cmd"), QJsonArray{QStringLiteral("fidan-session")}},
                  {QStringLiteral("env"), QJsonArray{QStringLiteral("XDG_SESSION_TYPE=wayland"),
                                                     QStringLiteral("XDG_CURRENT_DESKTOP=Fidan")}}});
        } else if (type == QStringLiteral("error")) {
            setError(response.value(QStringLiteral("description")).toString(tr("Login failed.")));
            m_busy = false;
        }
    }
}

void GreeterController::socketFailed(QLocalSocket::LocalSocketError error)
{
    Q_UNUSED(error)
    setError(m_socket.errorString());
}

void GreeterController::setError(const QString &message)
{
    m_error = message;
    emit stateChanged();
}

