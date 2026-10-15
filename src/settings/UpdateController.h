// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QProcess>

class UpdateController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool downloaded READ downloaded NOTIFY stateChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)

public:
    explicit UpdateController(QObject *parent = nullptr);

    bool busy() const { return m_process.state() != QProcess::NotRunning; }
    bool updateAvailable() const { return m_updateAvailable; }
    bool downloaded() const { return m_downloaded; }
    QString latestVersion() const { return m_latestVersion; }
    QString statusText() const { return m_statusText; }

    Q_INVOKABLE void check();
    Q_INVOKABLE void download();
    Q_INVOKABLE void install();

signals:
    void stateChanged();

private:
    enum class Operation { None, Check, Download, Install };
    void start(Operation operation, const QString &argument);
    void finish(int exitCode, QProcess::ExitStatus exitStatus);
    void setStatus(const QString &status);

    QProcess m_process;
    Operation m_operation = Operation::None;
    bool m_updateAvailable = false;
    bool m_downloaded = false;
    QString m_latestVersion;
    QString m_statusText;
};
