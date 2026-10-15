// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QVariantList>
#include <QProcess>
#include <QTimer>

class DisplayController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList outputs READ outputs NOTIFY outputsChanged)
    Q_PROPERTY(QString selectedName READ selectedName WRITE setSelectedName NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap currentOutput READ currentOutput NOTIFY selectionChanged)
    Q_PROPERTY(bool available READ available NOTIFY outputsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorChanged)
    Q_PROPERTY(bool confirmationPending READ confirmationPending NOTIFY confirmationChanged)
    Q_PROPERTY(int secondsRemaining READ secondsRemaining NOTIFY confirmationChanged)
    Q_PROPERTY(bool identifying READ identifying NOTIFY identifyingChanged)
    Q_PROPERTY(bool nightLightAvailable READ nightLightAvailable NOTIFY selectionChanged)
    Q_PROPERTY(bool nightLightEnabled READ nightLightEnabled NOTIFY colorChanged)
    Q_PROPERTY(int temperature READ temperature NOTIFY colorChanged)
    Q_PROPERTY(bool colorBusy READ colorBusy NOTIFY colorChanged)
public:
    explicit DisplayController(bool mock = false, QObject *parent = nullptr);
    ~DisplayController() override;
    QVariantList outputs() const { return m_outputs; }
    QString selectedName() const { return m_selected; }
    void setSelectedName(const QString &name);
    QVariantMap currentOutput() const;
    bool available() const { return m_available; }
    bool busy() const { return m_process.state() != QProcess::NotRunning; }
    QString errorText() const { return m_error; }
    bool confirmationPending() const { return m_pending; }
    int secondsRemaining() const { return m_seconds; }
    bool identifying() const { return m_identifying; }
    bool nightLightAvailable() const;
    bool nightLightEnabled() const { return m_nightEnabled; }
    int temperature() const { return m_temperature; }
    bool colorBusy() const { return m_color.state() != QProcess::NotRunning; }
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void identify();
    Q_INVOKABLE void apply(int width, int height, double refresh, double scale, const QString &transform);
    Q_INVOKABLE void keep();
    Q_INVOKABLE void revert();
    Q_INVOKABLE void setNightLight(bool enabled, int temperature);
signals:
    void outputsChanged();
    void selectionChanged();
    void busyChanged();
    void errorChanged();
    void confirmationChanged();
    void identifyingChanged();
    void colorChanged();
private:
    void start(const QString &phase, const QStringList &arguments);
    void finished(int code, QProcess::ExitStatus status);
    void loadOutputs(const QByteArray &json);
    void fallbackOutputs();
    void error(const QString &message);
    void loadColor();
    bool saveTarget();
    static QStringList arguments(const QVariantMap &output);
    QVariantList m_outputs;
    QString m_selected, m_error, m_phase;
    QVariantMap m_original, m_target;
    QProcess m_process, m_color;
    QTimer m_countdown, m_timeout;
    bool m_available = false, m_pending = false, m_identifying = false;
    bool m_mock = false, m_changed = false, m_revertRequested = false;
    int m_seconds = 0, m_temperature = 4500;
    bool m_nightEnabled = false;
};
