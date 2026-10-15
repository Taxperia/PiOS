// SPDX-License-Identifier: GPL-3.0-or-later
#include "SystemMonitor.h"

#include <QDateTime>
#include <QFile>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QTextStream>
#include <QTimer>

namespace {
QString humanBytes(quint64 bytes)
{
    constexpr double gib = 1024.0 * 1024.0 * 1024.0;
    return QString::number(bytes / gib, 'f', 1) + QStringLiteral(" GB");
}
}

SystemMonitor::SystemMonitor(bool mockMode, QObject *parent)
    : QObject(parent)
    , m_mockMode(mockMode)
    , m_deviceModel(mockMode ? QStringLiteral("Raspberry Pi 5") : readDeviceModel())
    , m_timer(new QTimer(this))
{
    m_timer->setInterval(1500);
    connect(m_timer, &QTimer::timeout, this, &SystemMonitor::refresh);
    refresh();
    m_timer->start();
}

void SystemMonitor::refresh()
{
    if (m_mockMode) {
        const qint64 tick = QDateTime::currentMSecsSinceEpoch() / 1500;
        m_cpuUsage = 16.0 + (tick % 7);
        m_memoryUsage = 31.0 + (tick % 3);
        m_storageUsage = 28.0;
        m_temperature = 41.0 + (tick % 3);
        m_temperatureAvailable = true;
        m_memoryLabel = QStringLiteral("2.6 GB / 8.0 GB");
        m_storageLabel = QStringLiteral("22 GB / 64 GB");
        emit metricsChanged();
        return;
    }

    refreshCpu();
    refreshMemory();
    refreshStorage();
    refreshTemperature();
    emit metricsChanged();
}

void SystemMonitor::refreshCpu()
{
    QFile file(QStringLiteral("/proc/stat"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    const QList<QByteArray> values = file.readLine().simplified().split(' ');
    if (values.size() < 5 || values.first() != "cpu") {
        return;
    }

    quint64 total = 0;
    for (qsizetype i = 1; i < values.size(); ++i) {
        total += values.at(i).toULongLong();
    }
    const quint64 idle = values.at(4).toULongLong() + (values.size() > 5 ? values.at(5).toULongLong() : 0);

    if (m_previousCpuTotal > 0 && total > m_previousCpuTotal) {
        const quint64 totalDelta = total - m_previousCpuTotal;
        const quint64 idleDelta = idle - m_previousCpuIdle;
        m_cpuUsage = 100.0 * static_cast<double>(totalDelta - idleDelta) / static_cast<double>(totalDelta);
    }
    m_previousCpuTotal = total;
    m_previousCpuIdle = idle;
}

void SystemMonitor::refreshMemory()
{
    QFile file(QStringLiteral("/proc/meminfo"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    quint64 totalKb = 0;
    quint64 availableKb = 0;
    for (const QByteArray &line : file.readAll().split('\n')) {
        if (line.startsWith("MemTotal:")) {
            totalKb = line.split(':').value(1).simplified().split(' ').value(0).toULongLong();
        } else if (line.startsWith("MemAvailable:")) {
            availableKb = line.split(':').value(1).simplified().split(' ').value(0).toULongLong();
        }
    }

    if (totalKb > 0) {
        const quint64 usedKb = totalKb - availableKb;
        m_memoryUsage = 100.0 * static_cast<double>(usedKb) / static_cast<double>(totalKb);
        m_memoryLabel = humanBytes(usedKb * 1024) + QStringLiteral(" / ") + humanBytes(totalKb * 1024);
    }
}

void SystemMonitor::refreshStorage()
{
    QStorageInfo storage = QStorageInfo::root();
    storage.refresh();
    if (!storage.isValid() || !storage.isReady() || storage.bytesTotal() <= 0
        || storage.bytesFree() < 0 || storage.bytesFree() > storage.bytesTotal()) {
        return;
    }

    const quint64 total = static_cast<quint64>(storage.bytesTotal());
    const quint64 used = total - static_cast<quint64>(storage.bytesFree());
    m_storageUsage = 100.0 * static_cast<double>(used) / static_cast<double>(total);
    m_storageLabel = humanBytes(used) + QStringLiteral(" / ") + humanBytes(total);
}

void SystemMonitor::refreshTemperature()
{
    m_temperatureAvailable = false;
    const QStringList candidates = {
        QStringLiteral("/sys/class/thermal/thermal_zone0/temp"),
        QStringLiteral("/sys/class/hwmon/hwmon0/temp1_input")
    };
    for (const QString &path : candidates) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            bool ok = false;
            const double raw = QString::fromUtf8(file.readAll()).trimmed().toDouble(&ok);
            if (ok && qIsFinite(raw)) {
                m_temperature = raw / 1000.0;
                m_temperatureAvailable = true;
                return;
            }
        }
    }
}

QString SystemMonitor::readDeviceModel()
{
    QFile file(QStringLiteral("/proc/device-tree/model"));
    if (file.open(QIODevice::ReadOnly)) {
        QString model = QString::fromUtf8(file.readAll());
        model.remove(QChar::Null);
        if (!model.trimmed().isEmpty()) {
            return model.trimmed();
        }
    }
    return QStringLiteral("Development system");
}

