// SPDX-License-Identifier: GPL-3.0-or-later
#include "UpdateController.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

UpdateController::UpdateController(QObject *parent)
    : QObject(parent)
    , m_statusText(tr("Updates have not been checked yet."))
{
    connect(&m_process, &QProcess::started, this, &UpdateController::stateChanged);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_operation = Operation::None;
            setStatus(tr("The update service could not be started."));
        }
    });
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &UpdateController::finish);
}

void UpdateController::check()
{
    start(Operation::Check, QStringLiteral("check"));
}

void UpdateController::download()
{
    if (m_updateAvailable)
        start(Operation::Download, QStringLiteral("download"));
}

void UpdateController::install()
{
    if (m_downloaded)
        start(Operation::Install, QStringLiteral("install"));
}

void UpdateController::start(Operation operation, const QString &argument)
{
    if (busy())
        return;

    QString executable = QStandardPaths::findExecutable(QStringLiteral("fidan-update"));
#ifdef FIDAN_SOURCE_DIR
    if (executable.isEmpty())
        executable = QStringLiteral(FIDAN_SOURCE_DIR "/scripts/fidan-update");
#endif
    m_operation = operation;
    switch (operation) {
    case Operation::Check:
        setStatus(tr("Checking GitHub releases…"));
        break;
    case Operation::Download:
        setStatus(tr("Downloading and verifying the signed update…"));
        break;
    case Operation::Install:
        setStatus(tr("Waiting for authorization to install…"));
        break;
    case Operation::None:
        return;
    }
    m_process.setProgram(executable);
    m_process.setArguments({argument});
    m_process.setProcessChannelMode(QProcess::SeparateChannels);
    m_process.start();
    emit stateChanged();
}

void UpdateController::finish(int exitCode, QProcess::ExitStatus exitStatus)
{
    const QByteArray output = m_process.readAllStandardOutput().trimmed();
    const QString error = QString::fromUtf8(m_process.readAllStandardError()).trimmed();
    const Operation completed = m_operation;
    m_operation = Operation::None;

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        setStatus(error.isEmpty() ? tr("The update operation failed.") : error);
        return;
    }

    if (completed == Operation::Check) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(output, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            setStatus(tr("The update service returned an invalid response."));
            return;
        }
        const QJsonObject result = document.object();
        m_updateAvailable = result.value(QStringLiteral("updateAvailable")).toBool();
        m_latestVersion = result.value(QStringLiteral("latest")).toString();
        m_downloaded = false;
        setStatus(m_updateAvailable
                      ? tr("Version %1 is available.").arg(m_latestVersion)
                      : tr("The system is up to date."));
        return;
    }

    if (completed == Operation::Download) {
        m_downloaded = true;
        setStatus(tr("The update was downloaded and its signature was verified."));
        return;
    }

    if (completed == Operation::Install) {
        m_downloaded = false;
        m_updateAvailable = false;
        setStatus(tr("The update was installed successfully. Restart when convenient."));
    }
}

void UpdateController::setStatus(const QString &status)
{
    m_statusText = status;
    emit stateChanged();
}
