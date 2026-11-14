// SPDX-License-Identifier: GPL-3.0-or-later
#include "DesktopServices.h"
#include "ShellController.h"
#include "NotificationService.h"
#include <QtTest>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusAbstractAdaptor>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <memory>
#include <QScopeGuard>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickStyle>

class FakePlayer final : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ status)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(bool CanControl READ yes)
    Q_PROPERTY(bool CanPlay READ yes)
    Q_PROPERTY(bool CanPause READ yes)
    Q_PROPERTY(bool CanGoNext READ yes)
    Q_PROPERTY(bool CanGoPrevious READ yes)
public:
    bool yes() const { return true; }
    QString status() const { return playing ? "Playing" : "Paused"; }
    QVariantMap metadata() const { return {{"xesam:title", "Real D-Bus track"}, {"xesam:artist", QStringList{"Test artist"}}}; }
    bool playing = false;
    int next = 0;
public slots:
    void PlayPause() { playing = !playing; }
    void Next() { ++next; }
    void Previous() { --next; }
};

class DesktopServicesTest final : public QObject {
    Q_OBJECT
    static QQuickItem *visual(QQuickItem *root, const QString &name) {
        if (root->objectName() == name) return root;
        for (auto *child : root->childItems()) if (auto *found = visual(child, name)) return found;
        return nullptr;
    }
    static void write(const QString &path, const QByteArray &data) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(data), qint64(data.size()));
    }
private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QQuickStyle::setStyle("Basic");
        qmlRegisterSingletonType(QUrl::fromLocalFile(QString(FIDAN_SOURCE_DIR) + "/src/shell/qml/Theme.qml"), "Fidan.Shell", 1, 0, "Theme");
    }
    void mockControls() {
        DesktopServices services(true);
        services.selectOutput("hdmi"); QCOMPARE(services.defaultOutput(), "hdmi");
        services.selectOutput("unknown"); QCOMPARE(services.defaultOutput(), "hdmi");
        services.setApplicationVolume(1, 500); QCOMPARE(services.audioStreams().at(0).toMap().value("volume").toInt(), 100);
        services.setApplicationVolume(1, -10); QCOMPARE(services.audioStreams().at(0).toMap().value("volume").toInt(), 0);
        services.toggleApplicationMute(1); QVERIFY(services.audioStreams().at(0).toMap().value("muted").toBool());
        QSignalSpy microphone(&services, &DesktopServices::microphoneOsdRequested);
        services.toggleMicrophoneMute(); QVERIFY(services.microphoneMuted()); QCOMPARE(microphone.count(), 1);
        services.toggleMicrophoneMute(); QVERIFY(!services.microphoneMuted());
        services.mediaAction("PlayPause"); QVERIFY(services.media().value("playing").toBool());
        services.mediaAction("Next"); QCOMPARE(services.media().value("title").toString(), services.tr("Evening breeze"));
        services.mediaAction("invalid"); QVERIFY(services.media().value("playing").toBool());
    }
    void mixerDoesNotResetDelegatesDuringDrag() {
        DesktopServices services(true);
        QSignalSpy changed(&services, &DesktopServices::audioChanged);
        services.setAudioEditing(true);
        services.setApplicationVolume(1, 35); services.setApplicationVolume(1, 65);
        QCOMPARE(changed.count(), 0);
        QCOMPARE(services.audioStreams().first().toMap().value("volume").toInt(), 65);
        services.setAudioEditing(false); QCOMPARE(changed.count(), 1);
    }
    void batteryThresholdsAndCharger() {
        QTemporaryDir power;
        write(power.filePath("BAT0/type"), "Battery"); write(power.filePath("BAT0/present"), "1");
        write(power.filePath("BAT0/capacity"), "60"); write(power.filePath("BAT0/status"), "Discharging");
        write(power.filePath("AC/type"), "Mains"); write(power.filePath("AC/online"), "0");
        DesktopServices services(false, nullptr, power.path());
        QSignalSpy notifications(&services, &DesktopServices::notificationRequested);
        services.refreshBattery(); QVERIFY(services.batteryAvailable()); QCOMPARE(services.batteryPercent(), 60); QCOMPARE(notifications.count(), 0);
        write(power.filePath("BAT0/capacity"), "19"); services.refreshBattery(); QCOMPARE(notifications.count(), 1);
        services.refreshBattery(); QCOMPARE(notifications.count(), 1);
        write(power.filePath("BAT0/capacity"), "9"); services.refreshBattery(); QCOMPARE(notifications.count(), 2);
        write(power.filePath("BAT0/capacity"), "4"); services.refreshBattery(); QCOMPARE(notifications.count(), 3);
        write(power.filePath("AC/online"), "1"); write(power.filePath("BAT0/status"), "Charging");
        services.refreshBattery(); QVERIFY(services.charging()); QVERIFY(services.pluggedIn()); QCOMPARE(notifications.count(), 4);
        write(power.filePath("AC/online"), "0"); write(power.filePath("BAT0/status"), "Discharging");
        services.refreshBattery(); QVERIFY(!services.charging()); QCOMPARE(notifications.count(), 6);
        write(power.filePath("BAT0/present"), "0"); services.refreshBattery(); QVERIFY(!services.batteryAvailable());
    }
    void weightedBatteries() {
        QTemporaryDir power;
        for (const auto &name : {"BAT0", "BAT1"}) {
            write(power.filePath(QString(name) + "/type"), "Battery");
            write(power.filePath(QString(name) + "/capacity"), "50");
        }
        write(power.filePath("BAT0/energy_now"), "10"); write(power.filePath("BAT0/energy_full"), "100");
        write(power.filePath("BAT1/energy_now"), "90"); write(power.filePath("BAT1/energy_full"), "300");
        DesktopServices services(false, nullptr, power.path()); services.refreshBattery(); QCOMPARE(services.batteryPercent(), 25);
    }
    void realMprisDispatchAndRemoval() {
        auto bus = QDBusConnection::sessionBus();
        const QString service = "org.mpris.MediaPlayer2.fidanTest";
        FakePlayer player;
        QVERIFY(bus.registerService(service));
        QVERIFY(bus.registerObject("/org/mpris/MediaPlayer2", &player, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties));
        const auto cleanup = qScopeGuard([&] { bus.unregisterObject("/org/mpris/MediaPlayer2"); bus.unregisterService(service); });
        QTemporaryDir power;
        DesktopServices services(false, nullptr, power.path());
        services.refreshMedia(); QTRY_COMPARE_WITH_TIMEOUT(services.mediaService(), service, 4000);
        QCOMPARE(services.media().value("title").toString(), "Real D-Bus track");
        QCOMPARE(services.media().value("artist").toString(), "Test artist");
        services.mediaAction("PlayPause"); QTRY_VERIFY(player.playing);
        QTRY_VERIFY(services.media().value("playing").toBool());
        services.mediaAction("Next"); QTRY_COMPARE(player.next, 1);
        bus.unregisterService(service); services.refreshMedia(); QTRY_VERIFY_WITH_TIMEOUT(services.mediaService().isEmpty(), 5000);
    }
    void asyncAudioCommandsAndDiscovery() {
        QTemporaryDir tools, power;
        const QByteArray oldPath = qgetenv("PATH"), oldLog = qgetenv("FIDAN_AUDIO_TEST_LOG");
        const auto restore = qScopeGuard([&] { qputenv("PATH", oldPath); qputenv("FIDAN_AUDIO_TEST_LOG", oldLog); });
        qputenv("PATH", tools.path().toUtf8() + ':' + oldPath);
        qputenv("FIDAN_AUDIO_TEST_LOG", tools.filePath("commands").toUtf8());
        write(tools.filePath("pactl"), R"SH(#!/bin/sh
printf '%s\n' "$*" >> "$FIDAN_AUDIO_TEST_LOG"
case "$*" in
get-default-sink) echo speakers ;;
get-default-source) echo microphone ;;
'--format=json list sinks') echo '[{"name":"speakers","description":"Speakers"},{"name":"hdmi","description":"HDMI"}]' ;;
'--format=json list sources') echo '[{"name":"microphone","monitor_of_sink":4294967295,"mute":false}]' ;;
'--format=json list sink-inputs') echo '[{"index":7,"properties":{"application.name":"Music"},"volume":{"left":{"value":32768},"right":{"value":32768}},"mute":false}]' ;;
esac
)SH");
        QFile script(tools.filePath("pactl")); QVERIFY(script.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        DesktopServices services(false, nullptr, power.path()); services.refreshAudio();
        QTRY_COMPARE_WITH_TIMEOUT(services.audioOutputs().size(), 2, 4000);
        QVERIFY(services.microphoneAvailable()); QCOMPARE(services.audioStreams().first().toMap().value("volume").toInt(), 50);
        services.selectOutput("hdmi");
        services.setApplicationVolume(7, 250); services.toggleMicrophoneMute();
        QFile log(tools.filePath("commands"));
        const auto commands = [&] { log.close(); log.open(QIODevice::ReadOnly); return log.readAll(); };
        QTRY_VERIFY_WITH_TIMEOUT(commands().contains("move-sink-input 7 hdmi"), 4000);
        QTRY_VERIFY_WITH_TIMEOUT(commands().contains("set-sink-input-volume 7 100%"), 4000);
        QTRY_VERIFY_WITH_TIMEOUT(commands().contains("set-source-mute microphone toggle"), 4000);
        QTest::qWait(150);
    }
    void screenshotCancelAndInvalidMode() {
        DesktopServices services(true);
        QSignalSpy notified(&services, &DesktopServices::notificationRequested);
        services.setScreenshotVisible(true); QVERIFY(services.screenshotVisible());
        services.captureScreenshot("invalid"); QVERIFY(services.screenshotVisible()); QCOMPARE(notified.count(), 0);
        services.setScreenshotVisible(false); QCOMPARE(notified.count(), 0);
        services.setScreenshotVisible(true); services.captureScreenshot("screen");
        QVERIFY(!services.screenshotVisible()); QCOMPARE(notified.count(), 1); QVERIFY(!services.screenshotBusy());
    }
    void readsBrightnessPercentageAndShowsIndicator() {
        QTemporaryDir tools;
        const QByteArray oldPath = qgetenv("PATH");
        const auto restore = qScopeGuard([&] { qputenv("PATH", oldPath); });
        write(tools.filePath("brightnessctl"), "#!/bin/sh\necho 'panel,backlight,450,45%,1000'\n");
        QFile executable(tools.filePath("brightnessctl"));
        QVERIFY(executable.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        qputenv("PATH", tools.path().toUtf8() + ':' + oldPath);
        ShellController controller(false, false);
        QVERIFY(controller.brightnessAvailable()); QCOMPARE(controller.brightness(), 45);
        controller.IncreaseBrightness(); QCOMPARE(controller.brightness(), 50);
        QCOMPARE(controller.osdKind(), "brightness"); QVERIFY(controller.volumeOsdVisible());
        QTest::qWait(80);
    }
    void qmlControlsAndLayout_data() {
        QTest::addColumn<QString>("component"); QTest::addColumn<bool>("dark");
        for (const auto &component : {"QuickSettings", "AudioMixer", "MediaCard", "ScreenshotToolbar", "VolumeOsd"})
            for (bool dark : {true, false}) QTest::newRow(qPrintable(QString(component) + (dark ? "-dark" : "-light"))) << QString(component) << dark;
    }
    void qmlControlsAndLayout() {
        QFETCH(QString, component); QFETCH(bool, dark);
        ShellController controller(true, true); controller.setDarkTheme(dark);
        DesktopServices services(true); NotificationService notifications;
        QQmlEngine engine; QStringList warnings;
        connect(&engine, &QQmlEngine::warnings, this, [&](const QList<QQmlError> &errors) { for (const auto &e : errors) warnings.append(e.toString()); });
        engine.rootContext()->setContextProperty("Controller", &controller);
        engine.rootContext()->setContextProperty("DesktopServices", &services);
        engine.rootContext()->setContextProperty("Notifications", &notifications);
        QQmlComponent source(&engine, QUrl::fromLocalFile(QString(FIDAN_SOURCE_DIR) + "/src/shell/qml/components/" + component + ".qml"));
        std::unique_ptr<QObject> object(source.create()); QVERIFY2(object, qPrintable(source.errorString()));
        auto *item = qobject_cast<QQuickItem *>(object.get()); QVERIFY(item);
        QQuickWindow window; window.resize(800, 760); window.setColor(dark ? QColor("#16241e") : QColor("#e7efea"));
        item->setParentItem(window.contentItem()); item->setWidth(qMax(292.0, item->implicitWidth()));
        item->setHeight(item->implicitHeight()); item->setX(20); item->setY(20);
        window.show(); QTest::qWait(120);
        QVERIFY(item->height() > 0); QVERIFY(item->height() <= window.height());
        if (component == "QuickSettings") {
            auto *button = item->findChild<QQuickItem *>("audioMixerToggle"); QVERIFY(button);
            QMetaObject::invokeMethod(button, "clicked"); QVERIFY(item->property("mixerExpanded").toBool());
            auto *mic = item->findChild<QQuickItem *>("microphoneToggle"); QVERIFY(mic);
            QMetaObject::invokeMethod(mic, "clicked"); QVERIFY(services.microphoneMuted());
        } else if (component == "AudioMixer") {
            auto *selector = visual(item, "audioOutputSelector"); QVERIFY(selector);
            QCOMPARE(selector->property("currentIndex").toInt(), 0);
            selector->forceActiveFocus(); QTest::keyClick(&window, Qt::Key_Space);
            QTest::keyClick(&window, Qt::Key_Down); QTest::keyClick(&window, Qt::Key_Return);
            QTRY_COMPARE(services.defaultOutput(), "headphones");
            QTest::qWait(150); // Wait for the ComboBox popup to return keyboard focus.
            auto *slider = visual(item, "applicationVolume-1"); QVERIFY(slider);
            slider->forceActiveFocus(); QTest::keyClick(&window, Qt::Key_Right);
            QTRY_COMPARE(services.audioStreams().first().toMap().value("volume").toInt(), 71);
        } else if (component == "MediaCard") {
            auto *button = item->findChild<QQuickItem *>("mediaPlayPause"); QVERIFY(button);
            button->forceActiveFocus(); QTest::keyClick(&window, Qt::Key_Space); QVERIFY(services.media().value("playing").toBool());
        } else if (component == "ScreenshotToolbar") {
            services.setScreenshotVisible(true); item->setProperty("activeShortcuts", true);
            QTest::keyClick(&window, Qt::Key_Escape); QTRY_VERIFY(!services.screenshotVisible());
        } else if (component == "VolumeOsd") {
            controller.setBrightness(42); QCOMPARE(item->property("level").toInt(), 42);
            controller.showIndicator("microphone"); services.toggleMicrophoneMute();
            QVERIFY(item->property("silent").toBool()); QCOMPARE(item->property("level").toInt(), 0);
            services.toggleMicrophoneMute(); QCOMPARE(item->property("level").toInt(), 100);
        }
        const QString capture = qEnvironmentVariable("FIDAN_SERVICES_CAPTURE_DIR");
        if (!capture.isEmpty()) { QDir().mkpath(capture); QTest::qWait(120); QVERIFY(window.grabWindow().save(capture + '/' + QTest::currentDataTag() + ".png")); }
        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join('\n')));
        item->setParentItem(nullptr);
    }
};
QTEST_MAIN(DesktopServicesTest)
#include "tst_desktop_services.moc"
