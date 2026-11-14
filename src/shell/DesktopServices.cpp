// SPDX-License-Identifier: GPL-3.0-or-later
#include "DesktopServices.h"
#include "../common/DesktopLaunch.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <memory>
#include <utility>

namespace {
int audioLevel(const QJsonObject &channels)
{
    double total = 0;
    for (const auto &channel : channels) {
        const auto value = channel.toObject();
        total += value.contains("value") ? value.value("value").toDouble() * 100.0 / 65536.0
                                        : value.value("value_percent").toString().remove('%').toDouble();
    }
    return channels.isEmpty() ? 0 : std::clamp(qRound(total / channels.size()), 0, 100);
}
QString readFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()).trimmed() : QString();
}
QVariantMap dbusMap(const QVariant &value)
{
    return value.metaType() == QMetaType::fromType<QDBusArgument>()
        ? qdbus_cast<QVariantMap>(value.value<QDBusArgument>()) : value.toMap();
}
}

DesktopServices::DesktopServices(bool mockMode, QObject *parent, const QString &batteryRoot)
    : QObject(parent), m_mockMode(mockMode), m_batteryRoot(batteryRoot)
{
    if (m_mockMode) {
        m_audioOutputs = {QVariantMap{{"id", "speakers"}, {"name", tr("Speakers")}},
                          QVariantMap{{"id", "headphones"}, {"name", tr("Headphones")}},
                          QVariantMap{{"id", "hdmi"}, {"name", "HDMI"}}};
        m_defaultOutput = "speakers";
        m_audioStreams = {QVariantMap{{"id", 1}, {"name", "Chromium"}, {"volume", 70}, {"muted", false}},
                          QVariantMap{{"id", 2}, {"name", "VLC"}, {"volume", 45}, {"muted", false}}};
        m_microphoneAvailable = true;
        m_mediaService = "org.mpris.MediaPlayer2.preview";
        m_mediaPlayers = {QVariantMap{{"id", m_mediaService}, {"name", tr("Preview player")}}};
        m_media = {{"title", tr("Mountain morning")}, {"artist", tr("Preview audio")},
                   {"player", tr("Preview player")}, {"playing", false}, {"art", ""},
                   {"canPlay", true}, {"canPause", true}, {"canNext", true}, {"canPrevious", true}};
        updateBattery(true, 76, false, false);
        return;
    }
    auto *timer = new QTimer(this);
    timer->setInterval(4000);
    connect(timer, &QTimer::timeout, this, [this] { refreshAudio(); refreshMedia(); });
    timer->start();
    auto *batteryTimer = new QTimer(this);
    batteryTimer->setInterval(15000);
    connect(batteryTimer, &QTimer::timeout, this, &DesktopServices::refreshBattery);
    batteryTimer->start();
    auto *watcher = new QDBusServiceWatcher("", QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForOwnerChange, this);
    watcher->addWatchedService("org.mpris.MediaPlayer2.*");
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, [this] { refreshMedia(); });
    QTimer::singleShot(0, this, [this] { refreshAudio(); refreshMedia(); refreshBattery(); });
}

void DesktopServices::run(const QString &program, const QStringList &arguments, Completion completion, int timeout)
{
    auto *process = new QProcess(this);
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("QT_WAYLAND_SHELL_INTEGRATION", "xdg-shell");
    process->setProcessEnvironment(environment);
    auto *timer = new QTimer(process);
    timer->setSingleShot(true);
    auto done = std::make_shared<bool>(false);
    const auto finish = [process, timer, completion, done](bool ok) {
        if (std::exchange(*done, true)) return;
        timer->stop();
        completion(ok, ok ? process->readAllStandardOutput() : process->readAllStandardError());
        process->deleteLater();
    };
    connect(process, &QProcess::finished, this, [finish](int code, QProcess::ExitStatus status) {
        finish(code == 0 && status == QProcess::NormalExit);
    });
    connect(process, &QProcess::errorOccurred, this, [finish](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) finish(false);
    });
    connect(timer, &QTimer::timeout, process, [process] { process->kill(); });
    timer->start(timeout);
    process->start(program, arguments);
}

void DesktopServices::refreshAudio()
{
    if (m_mockMode || m_audioRefreshing || m_audioWriting || m_audioEditing) return;
    m_audioRefreshing = true;
    struct Snapshot { QString output, source; QJsonArray sinks, sources, streams; };
    auto snapshot = std::make_shared<Snapshot>();
    run("pactl", {"get-default-sink"}, [this, snapshot](bool ok, const QByteArray &data) {
        if (!ok) {
            m_audioRefreshing = false;
            m_audioOutputs.clear(); m_audioStreams.clear(); m_defaultOutput.clear();
            m_microphoneAvailable = false;
            emit audioChanged(); processAudioQueue(); return;
        }
        snapshot->output = QString::fromUtf8(data).trimmed();
        run("pactl", {"get-default-source"}, [this, snapshot](bool, const QByteArray &data) {
            snapshot->source = QString::fromUtf8(data).trimmed();
            run("pactl", {"--format=json", "list", "sinks"}, [this, snapshot](bool, const QByteArray &data) {
                snapshot->sinks = QJsonDocument::fromJson(data).array();
                run("pactl", {"--format=json", "list", "sources"}, [this, snapshot](bool, const QByteArray &data) {
                    snapshot->sources = QJsonDocument::fromJson(data).array();
                    run("pactl", {"--format=json", "list", "sink-inputs"}, [this, snapshot](bool, const QByteArray &data) {
                        snapshot->streams = QJsonDocument::fromJson(data).array();
                        if (m_audioEditing) { m_audioRefreshing = false; processAudioQueue(); return; }
                        QVariantList outputs, streams;
                        for (const auto &item : snapshot->sinks) {
                            const auto sink = item.toObject();
                            outputs.append(QVariantMap{{"id", sink.value("name").toString()},
                                {"name", sink.value("description").toString(sink.value("name").toString())}});
                        }
                        for (const auto &item : snapshot->streams) {
                            const auto stream = item.toObject();
                            const auto properties = stream.value("properties").toObject();
                            streams.append(QVariantMap{{"id", stream.value("index").toInt()},
                                {"name", properties.value("application.name").toString(properties.value("media.name").toString(tr("Application")))},
                                {"volume", audioLevel(stream.value("volume").toObject())}, {"muted", stream.value("mute").toBool()}});
                        }
                        bool microphoneAvailable = false, microphoneMuted = false;
                        for (const auto &item : snapshot->sources) {
                            const auto source = item.toObject();
                            if (source.value("name").toString() == snapshot->source
                                && !snapshot->source.endsWith(".monitor")
                                && source.value("monitor_of_sink").toDouble(4294967295.0) == 4294967295.0) {
                                microphoneAvailable = true;
                                microphoneMuted = source.value("mute").toBool();
                            }
                        }
                        const bool changed = m_audioOutputs != outputs || m_audioStreams != streams
                            || m_defaultOutput != snapshot->output || m_defaultSource != snapshot->source
                            || m_microphoneAvailable != microphoneAvailable || m_microphoneMuted != microphoneMuted;
                        m_microphoneAvailable = microphoneAvailable; m_microphoneMuted = microphoneMuted;
                        m_audioOutputs = outputs; m_audioStreams = streams;
                        m_defaultOutput = snapshot->output; m_defaultSource = snapshot->source;
                        m_audioRefreshing = false;
                        if (changed) emit audioChanged();
                        processAudioQueue();
                    });
                });
            });
        });
    });
}

void DesktopServices::enqueueAudio(const QStringList &arguments, std::function<void()> success)
{
    // Replace pending slider values for the same stream while retaining command order.
    for (auto &pending : m_audioQueue) {
        if (arguments.value(0) == "set-sink-input-volume" && pending.first.mid(0, 2) == arguments.mid(0, 2)) {
            pending = {arguments, success}; return;
        }
    }
    m_audioQueue.enqueue({arguments, success});
    processAudioQueue();
}

void DesktopServices::processAudioQueue()
{
    if (m_audioWriting || m_audioRefreshing || m_audioQueue.isEmpty()) return;
    const auto command = m_audioQueue.dequeue();
    m_audioWriting = true;
    run("pactl", command.first, [this, command](bool ok, const QByteArray &error) {
        m_audioWriting = false;
        if (ok && command.second) command.second();
        if (!ok) emit actionFailed(tr("Audio change failed. %1").arg(QString::fromUtf8(error).trimmed().left(180)));
        if (m_audioQueue.isEmpty()) refreshAudio(); else processAudioQueue();
    });
}

void DesktopServices::selectOutput(const QString &name)
{
    const bool found = std::any_of(m_audioOutputs.begin(), m_audioOutputs.end(), [&name](const QVariant &v) { return v.toMap().value("id").toString() == name; });
    if (!found || name == m_defaultOutput) return;
    if (m_mockMode) { m_defaultOutput = name; emit audioChanged(); return; }
    enqueueAudio({"set-default-sink", name}, [this, name] {
        m_defaultOutput = name; emit audioChanged();
        for (const auto &item : m_audioStreams)
            enqueueAudio({"move-sink-input", QString::number(item.toMap().value("id").toInt()), name});
    });
}

void DesktopServices::setApplicationVolume(int id, int volume)
{
    volume = std::clamp(volume, 0, 100);
    for (auto &item : m_audioStreams) {
        auto stream = item.toMap();
        if (stream.value("id").toInt() != id) continue;
        if (m_mockMode) { stream["volume"] = volume; item = stream; if (!m_audioEditing) emit audioChanged(); }
        else enqueueAudio({"set-sink-input-volume", QString::number(id), QString::number(volume) + '%'});
        return;
    }
}

void DesktopServices::setAudioEditing(bool editing)
{
    if (m_audioEditing == editing) return;
    m_audioEditing = editing;
    if (!editing) {
        if (m_mockMode) emit audioChanged();
        else refreshAudio();
    }
}

void DesktopServices::toggleApplicationMute(int id)
{
    for (auto &item : m_audioStreams) {
        auto stream = item.toMap();
        if (stream.value("id").toInt() != id) continue;
        if (m_mockMode) { stream["muted"] = !stream.value("muted").toBool(); item = stream; emit audioChanged(); }
        else enqueueAudio({"set-sink-input-mute", QString::number(id), "toggle"});
        return;
    }
}

void DesktopServices::toggleMicrophoneMute()
{
    if (!m_microphoneAvailable) { emit actionFailed(tr("No microphone is available.")); return; }
    const auto changed = [this] {
        m_microphoneMuted = !m_microphoneMuted;
        emit audioChanged(); emit microphoneOsdRequested();
    };
    if (m_mockMode) changed();
    else enqueueAudio({"set-source-mute", m_defaultSource, "toggle"}, changed);
}

void DesktopServices::refreshMedia()
{
    if (m_mockMode || m_mediaRefreshing || !QDBusConnection::sessionBus().interface()) return;
    m_mediaRefreshing = true;
    auto *names = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().interface()->asyncCall("ListNames"), this);
    connect(names, &QDBusPendingCallWatcher::finished, this, [this, names] {
        const QDBusPendingReply<QStringList> reply = *names;
        names->deleteLater();
        QStringList services;
        if (!reply.isError()) for (const auto &name : reply.value())
            if (name.startsWith("org.mpris.MediaPlayer2.")) services.append(name);
        services.sort();
        if (services.isEmpty()) {
            m_mediaPlayers.clear(); m_mediaService.clear(); m_media.clear();
            m_mediaRefreshing = false; emit mediaChanged(); return;
        }
        struct Players { int pending; QMap<QString, QVariantMap> values; };
        auto players = std::make_shared<Players>(); players->pending = services.size();
        for (const auto &service : services) {
            auto call = QDBusMessage::createMethodCall(service, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "GetAll");
            call << QStringLiteral("org.mpris.MediaPlayer2.Player");
            auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(call, 1200), this);
            connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, service, players] {
                const QDBusPendingReply<QVariantMap> result = *watcher;
                watcher->deleteLater();
                if (!result.isError()) players->values.insert(service, result.value());
                if (--players->pending) return;
                m_mediaPlayers.clear();
                QString playing;
                for (auto it = players->values.cbegin(); it != players->values.cend(); ++it) {
                    m_mediaPlayers.append(QVariantMap{{"id", it.key()}, {"name", it.key().mid(23)}});
                    if (it.value().value("PlaybackStatus").toString() == "Playing" && playing.isEmpty()) playing = it.key();
                }
                if (!m_playerPinned || !players->values.contains(m_mediaService)) {
                    if (!playing.isEmpty()) m_mediaService = playing;
                    else if (!players->values.contains(m_mediaService)) m_mediaService = players->values.isEmpty() ? QString() : players->values.firstKey();
                }
                const auto properties = players->values.value(m_mediaService);
                const auto metadata = dbusMap(properties.value("Metadata"));
                const QUrl art(metadata.value("mpris:artUrl").toString());
                const bool control = properties.value("CanControl", false).toBool();
                m_media = properties.isEmpty() ? QVariantMap() : QVariantMap{
                    {"title", metadata.value("xesam:title", tr("Unknown track"))},
                    {"artist", metadata.value("xesam:artist").toStringList().join(", ")},
                    {"player", m_mediaService.mid(23)},
                    {"art", QStringList{"file", "http", "https"}.contains(art.scheme()) ? art.toString() : QString()},
                    {"playing", properties.value("PlaybackStatus").toString() == "Playing"},
                    {"canPlay", control && properties.value("CanPlay").toBool()},
                    {"canPause", control && properties.value("CanPause").toBool()},
                    {"canNext", control && properties.value("CanGoNext").toBool()},
                    {"canPrevious", control && properties.value("CanGoPrevious").toBool()}};
                if (m_media.isEmpty()) m_mediaService.clear();
                m_mediaRefreshing = false; emit mediaChanged();
            });
        }
    });
}

void DesktopServices::selectPlayer(const QString &service)
{
    for (const auto &player : m_mediaPlayers) if (player.toMap().value("id").toString() == service) {
        m_mediaService = service; m_playerPinned = true; refreshMedia(); return;
    }
}

void DesktopServices::mediaAction(const QString &action)
{
    QString capability;
    if (action == "PlayPause") capability = m_media.value("playing").toBool() ? "canPause" : "canPlay";
    else if (action == "Next") capability = "canNext";
    else if (action == "Previous") capability = "canPrevious";
    else return;
    if (m_mediaService.isEmpty() || !m_media.value(capability).toBool()) return;
    if (m_mockMode) {
        if (action == "PlayPause") m_media["playing"] = !m_media.value("playing").toBool();
        else m_media["title"] = action == "Next" ? tr("Evening breeze") : tr("Mountain morning");
        emit mediaChanged(); return;
    }
    const auto call = QDBusMessage::createMethodCall(m_mediaService, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", action);
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(call, 1500), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        const QDBusPendingReply<> reply = *watcher;
        if (reply.isError()) emit actionFailed(tr("The media player could not complete the action."));
        watcher->deleteLater(); refreshMedia();
    });
}

void DesktopServices::refreshBattery()
{
    if (m_mockMode) return;
    int count = 0, percentage = 0;
    double energyNow = 0, energyFull = 0;
    bool charging = false, plugged = false, allEnergy = true;
    const QDir supplies(m_batteryRoot);
    for (const auto &entry : supplies.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString base = supplies.filePath(entry) + '/';
        if (readFile(base + "type") != "Battery") { plugged |= readFile(base + "online") == "1"; continue; }
        if (readFile(base + "present") == "0") continue;
        bool ok = false;
        const int capacity = readFile(base + "capacity").toInt(&ok);
        if (!ok) continue;
        ++count; percentage += std::clamp(capacity, 0, 100);
        charging |= readFile(base + "status") == "Charging";
        const double full = readFile(base + "energy_full").toDouble();
        const double now = readFile(base + "energy_now").toDouble();
        if (full > 0) { energyFull += full; energyNow += now; } else allEnergy = false;
    }
    const int level = allEnergy && energyFull > 0 ? qRound(100 * energyNow / energyFull) : count ? percentage / count : 0;
    updateBattery(count > 0, std::clamp(level, 0, 100), charging, plugged || charging);
}

void DesktopServices::updateBattery(bool available, int percentage, bool charging, bool plugged)
{
    if (m_batteryInitialized && available && m_batteryAvailable && plugged != m_pluggedIn)
        emit notificationRequested(plugged ? tr("Power connected") : tr("Running on battery"),
            plugged ? tr("External power is connected.") : tr("Battery level: %1%").arg(percentage));
    if (!available || plugged || percentage > 25) m_batteryWarning = 0;
    if (available && !plugged) {
        const int warning = percentage <= 5 ? 3 : percentage <= 10 ? 2 : percentage <= 20 ? 1 : 0;
        if (warning > m_batteryWarning) {
            emit notificationRequested(warning >= 2 ? tr("Battery critically low") : tr("Battery low"),
                tr("%1% remaining. Connect a charger.").arg(percentage));
            m_batteryWarning = warning;
        }
    }
    m_batteryInitialized = true;
    if (m_batteryAvailable == available && m_batteryPercent == percentage && m_charging == charging && m_pluggedIn == plugged) return;
    m_batteryAvailable = available; m_batteryPercent = percentage;
    m_charging = charging; m_pluggedIn = plugged;
    emit batteryChanged();
}

void DesktopServices::setScreenshotVisible(bool visible)
{
    if (m_screenshotBusy || m_screenshotVisible == visible) return;
    m_screenshotVisible = visible; emit screenshotVisibleChanged();
}

void DesktopServices::captureScreenshot(const QString &mode)
{
    if (m_screenshotBusy || !QStringList{"screen", "region", "window"}.contains(mode)) return;
    setScreenshotVisible(false);
    if (m_mockMode) {
        emit notificationRequested(tr("Screenshot preview"), tr("Screenshots are saved in a real desktop session.")); return;
    }
    m_screenshotBusy = true; emit screenshotBusyChanged();
    QTimer::singleShot(240, this, [this, mode] {
        QStringList arguments;
        if (mode == "region") arguments << "--region";
        else if (mode == "window") arguments << "--window";
        run("fidan-screenshot", arguments, [this](bool ok, const QByteArray &) {
            m_screenshotBusy = false; emit screenshotBusyChanged();
            if (!ok) emit actionFailed(tr("Screenshot failed. Check that grim and slurp are installed."));
        }, 120000);
    });
}
