// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QQueue>
#include <QVariantList>
#include <functional>

class DesktopServices final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList audioOutputs READ audioOutputs NOTIFY audioChanged)
    Q_PROPERTY(QVariantList audioStreams READ audioStreams NOTIFY audioChanged)
    Q_PROPERTY(QString defaultOutput READ defaultOutput NOTIFY audioChanged)
    Q_PROPERTY(bool microphoneAvailable READ microphoneAvailable NOTIFY audioChanged)
    Q_PROPERTY(bool microphoneMuted READ microphoneMuted NOTIFY audioChanged)
    Q_PROPERTY(QVariantList mediaPlayers READ mediaPlayers NOTIFY mediaChanged)
    Q_PROPERTY(QString mediaService READ mediaService NOTIFY mediaChanged)
    Q_PROPERTY(QVariantMap media READ media NOTIFY mediaChanged)
    Q_PROPERTY(bool batteryAvailable READ batteryAvailable NOTIFY batteryChanged)
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY batteryChanged)
    Q_PROPERTY(bool charging READ charging NOTIFY batteryChanged)
    Q_PROPERTY(bool pluggedIn READ pluggedIn NOTIFY batteryChanged)
    Q_PROPERTY(bool screenshotVisible READ screenshotVisible WRITE setScreenshotVisible NOTIFY screenshotVisibleChanged)
    Q_PROPERTY(bool screenshotBusy READ screenshotBusy NOTIFY screenshotBusyChanged)

public:
    explicit DesktopServices(bool mockMode, QObject *parent = nullptr,
                             const QString &batteryRoot = QStringLiteral("/sys/class/power_supply"));
    QVariantList audioOutputs() const { return m_audioOutputs; }
    QVariantList audioStreams() const { return m_audioStreams; }
    QString defaultOutput() const { return m_defaultOutput; }
    bool microphoneAvailable() const { return m_microphoneAvailable; }
    bool microphoneMuted() const { return m_microphoneMuted; }
    QVariantList mediaPlayers() const { return m_mediaPlayers; }
    QString mediaService() const { return m_mediaService; }
    QVariantMap media() const { return m_media; }
    bool batteryAvailable() const { return m_batteryAvailable; }
    int batteryPercent() const { return m_batteryPercent; }
    bool charging() const { return m_charging; }
    bool pluggedIn() const { return m_pluggedIn; }
    bool screenshotVisible() const { return m_screenshotVisible; }
    bool screenshotBusy() const { return m_screenshotBusy; }
    void setScreenshotVisible(bool visible);

    Q_INVOKABLE void refreshAudio();
    Q_INVOKABLE void selectOutput(const QString &name);
    Q_INVOKABLE void setApplicationVolume(int id, int volume);
    Q_INVOKABLE void setAudioEditing(bool editing);
    Q_INVOKABLE void toggleApplicationMute(int id);
    Q_INVOKABLE void toggleMicrophoneMute();
    Q_INVOKABLE void selectPlayer(const QString &service);
    Q_INVOKABLE void mediaAction(const QString &action);
    Q_INVOKABLE void refreshMedia();
    Q_INVOKABLE void refreshBattery();
    Q_INVOKABLE void captureScreenshot(const QString &mode);

signals:
    void audioChanged();
    void mediaChanged();
    void batteryChanged();
    void microphoneOsdRequested();
    void screenshotVisibleChanged();
    void screenshotBusyChanged();
    void notificationRequested(const QString &title, const QString &message);
    void actionFailed(const QString &message);

private:
    using Completion = std::function<void(bool, const QByteArray &)>;
    void run(const QString &program, const QStringList &arguments, Completion completion, int timeout = 2500);
    void enqueueAudio(const QStringList &arguments, std::function<void()> success = {});
    void processAudioQueue();
    void updateBattery(bool available, int percentage, bool charging, bool plugged);
    bool m_mockMode;
    QString m_batteryRoot;
    QVariantList m_audioOutputs, m_audioStreams, m_mediaPlayers;
    QString m_defaultOutput, m_defaultSource, m_mediaService;
    QVariantMap m_media;
    bool m_microphoneAvailable = false, m_microphoneMuted = false;
    bool m_audioRefreshing = false, m_audioWriting = false, m_mediaRefreshing = false;
    bool m_audioEditing = false;
    bool m_playerPinned = false;
    QQueue<QPair<QStringList, std::function<void()>>> m_audioQueue;
    bool m_batteryAvailable = false, m_charging = false, m_pluggedIn = false;
    bool m_batteryInitialized = false;
    int m_batteryPercent = 0, m_batteryWarning = 0;
    bool m_screenshotVisible = false, m_screenshotBusy = false;
};
