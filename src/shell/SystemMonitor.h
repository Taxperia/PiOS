// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>

class QTimer;

class SystemMonitor final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY metricsChanged)
    Q_PROPERTY(double memoryUsage READ memoryUsage NOTIFY metricsChanged)
    Q_PROPERTY(double storageUsage READ storageUsage NOTIFY metricsChanged)
    Q_PROPERTY(double temperature READ temperature NOTIFY metricsChanged)
    Q_PROPERTY(bool temperatureAvailable READ temperatureAvailable NOTIFY metricsChanged)
    Q_PROPERTY(QString memoryLabel READ memoryLabel NOTIFY metricsChanged)
    Q_PROPERTY(QString storageLabel READ storageLabel NOTIFY metricsChanged)
    Q_PROPERTY(QString deviceModel READ deviceModel CONSTANT)

public:
    explicit SystemMonitor(bool mockMode, QObject *parent = nullptr);

    double cpuUsage() const { return m_cpuUsage; }
    double memoryUsage() const { return m_memoryUsage; }
    double storageUsage() const { return m_storageUsage; }
    double temperature() const { return m_temperature; }
    bool temperatureAvailable() const { return m_temperatureAvailable; }
    QString memoryLabel() const { return m_memoryLabel; }
    QString storageLabel() const { return m_storageLabel; }
    QString deviceModel() const { return m_deviceModel; }

public slots:
    void refresh();

signals:
    void metricsChanged();

private:
    void refreshCpu();
    void refreshMemory();
    void refreshStorage();
    void refreshTemperature();
    static QString readDeviceModel();

    bool m_mockMode = false;
    quint64 m_previousCpuTotal = 0;
    quint64 m_previousCpuIdle = 0;
    double m_cpuUsage = 0.0;
    double m_memoryUsage = 0.0;
    double m_storageUsage = 0.0;
    double m_temperature = 0.0;
    bool m_temperatureAvailable = false;
    QString m_memoryLabel;
    QString m_storageLabel;
    QString m_deviceModel;
    QTimer *m_timer = nullptr;
};

