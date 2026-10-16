// SPDX-License-Identifier: GPL-3.0-or-later
#include "UpdateController.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

UpdateController::UpdateController(QObject *parent)
    : QObject(parent)
    , m_statusText(tr("Güncellemeler henüz kontrol edilmedi."))
{
    connect(&m_process, &QProcess::started, this, &UpdateController::stateChanged);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_operation = Operation::None;
            setStatus(tr("Güncelleme hizmeti başlatılamadı."));
        }
    });
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &UpdateController::finish);
}

UpdateController::~UpdateController()
{
    m_refreshTimer.stop();
    disconnect(&m_process, nullptr, this, nullptr);
    if (busy()) { m_process.kill(); m_process.waitForFinished(1000); }
}

void UpdateController::startAutomaticChecks()
{
    if (m_refreshTimer.isActive())
        return;
    connect(&m_refreshTimer, &QTimer::timeout, this, &UpdateController::check);
    m_refreshTimer.start(30 * 60 * 1000);
    QTimer::singleShot(1500, this, &UpdateController::check);
}

void UpdateController::check()
{
    start(Operation::Check, QStringLiteral("check"));
}

void UpdateController::download()
{
    if (m_updateAvailable && m_installable)
        start(Operation::Download, QStringLiteral("download"));
}

void UpdateController::install()
{
    if (m_downloaded)
        start(Operation::Install, QStringLiteral("install"));
}

void UpdateController::setAutomaticEnabled(bool enabled)
{
    if (busy())
        return;
    m_requestedAutomatic = enabled;
    start(Operation::Configure, QStringLiteral("configure"));
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
        setStatus(tr("GitHub sürümleri kontrol ediliyor…"));
        break;
    case Operation::Download:
        setStatus(tr("Güncelleme indiriliyor ve imzası doğrulanıyor…"));
        break;
    case Operation::Install:
        setStatus(tr("Kurulum için yetkilendirme bekleniyor…"));
        break;
    case Operation::Configure:
        setStatus(tr("Otomatik güncelleme tercihi için yetkilendirme bekleniyor…"));
        break;
    case Operation::None:
        return;
    }
    m_process.setProgram(executable);
    m_process.setArguments({argument});
    if (operation == Operation::Configure)
        m_process.setArguments({argument, m_requestedAutomatic ? QStringLiteral("on") : QStringLiteral("off")});
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
        setStatus(error.isEmpty() ? tr("Güncelleme işlemi başarısız oldu.") : error);
        return;
    }

    if (completed == Operation::Check) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(output, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            setStatus(tr("Güncelleme hizmeti geçersiz bir yanıt döndürdü."));
            return;
        }
        const QJsonObject result = document.object();
        m_updateAvailable = result.value(QStringLiteral("updateAvailable")).toBool();
        m_latestVersion = result.value(QStringLiteral("latest")).toString();
        m_downloaded = result.value(QStringLiteral("downloaded")).toBool();
        m_automaticEnabled = result.value(QStringLiteral("automaticEnabled")).toBool();
        m_installable = result.value(QStringLiteral("installable")).toBool();
        setStatus(m_updateAvailable
                      ? (m_installable ? tr("%1 sürümü kullanılabilir.").arg(m_latestVersion) : tr("%1 sürümünün imzalı paketleri henüz hazır değil.").arg(m_latestVersion))
                      : (m_latestVersion.isEmpty() ? tr("Bu kanal için henüz sürüm yayınlanmadı.") : tr("PiOS güncel. Sürüm: %1").arg(m_latestVersion)));
        return;
    }

    if (completed == Operation::Download) {
        m_downloaded = QJsonDocument::fromJson(output).object().value(QStringLiteral("downloaded")).toBool();
        setStatus(m_downloaded ? tr("Güncelleme indirildi ve imzası doğrulandı.") : tr("İndirilecek daha yeni bir PiOS sürümü yok."));
        return;
    }

    if (completed == Operation::Configure) {
        m_automaticEnabled = m_requestedAutomatic;
        setStatus(m_automaticEnabled ? tr("Otomatik güncellemeler açıldı.") : tr("Otomatik güncellemeler kapatıldı."));
        return;
    }

    if (completed == Operation::Install) {
        m_downloaded = false;
        m_updateAvailable = false;
        setStatus(tr("Güncelleme kuruldu. Yeni masaüstünü sonraki oturumda kullanabilirsin."));
    }
}

void UpdateController::setStatus(const QString &status)
{
    m_statusText = status;
    emit stateChanged();
}
