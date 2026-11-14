// SPDX-License-Identifier: GPL-3.0-or-later
#include "FirstBootService.h"
#include "FirstBootClient.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickStyle>
#include <QTranslator>
#include <QQmlComponent>
#include <memory>

class FirstBootTest final : public QObject {
    Q_OBJECT
    QTemporaryDir root;
    bool user = false, passwordFails = false, enableFails = false, homeFails = false, profileFails = false;
    QList<QStringList> commands;
    QByteArray profileInput;
    std::unique_ptr<FirstBootService> setup;
    void write(const QString &path, const QByteArray &bytes) {
        QVERIFY(QDir().mkpath(QFileInfo(root.path() + path).absolutePath()));
        QFile file(root.path() + path); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(bytes), qint64(bytes.size()));
    }
    FirstBootService::Result execute(const QString &program, const QStringList &args, const QByteArray &input) {
        commands.append(QStringList{program} + args);
        if (args.contains("/usr/bin/fidan-setup-preferences")) {profileInput=input;return {!profileFails,{}};}
        if (program.endsWith("getent") && args.first() == "passwd") return {user, {}};
        if (program.endsWith("useradd")) user = true;
        if (program.endsWith("chpasswd") && passwordFails) return {false, {}};
        if (program.endsWith("mkhomedir_helper") && homeFails) return {false, {}};
        if (program.endsWith("systemctl") && args.contains("greetd.service") && enableFails) return {false, {}};
        if (program.endsWith("nmcli")) return {true, "Home Wi-Fi\nWorkshop\nHome Wi-Fi\n"};
        return {true, {}};
    }
    void reload() { setup = std::make_unique<FirstBootService>(root.path(), [this](const auto &p, const auto &a, const auto &i) { return execute(p,a,i); }, [] { return true; }); }
    void account() {
        QVERIFY(setup->SetLocale("tr_TR.UTF-8", "trq", "Europe/Istanbul"));
        QVERIFY(setup->CreateUser("maker", "Maker", "a-long-test-password"));
    }
private slots:
    void initTestCase() { QQuickStyle::setStyle("Basic"); }
    void init() {
        user = false; passwordFails = false; enableFails = false; homeFails = false; profileFails=false; commands.clear();profileInput.clear();
        QFile::remove(root.path() + "/var/lib/fidan/setup-state.json"); QFile::remove(root.path() + "/var/lib/fidan/firstboot-complete");
        write("/usr/share/zoneinfo/Europe/Istanbul", "timezone"); write("/usr/share/zoneinfo/UTC", "timezone"); reload();
    }
    void cleanup() { setup.reset(); }
    void cannotFinishBeforeAllStepsAndLoginReady() {
        QVERIFY(!setup->Complete()); account(); QVERIFY(setup->SkipNetwork()); QVERIFY(setup->SetPrivacy(false, false)); QVERIFY(!setup->Complete());
        QVERIFY(setup->SkipUpdates()); enableFails = true; QVERIFY(!setup->Complete()); QVERIFY(!setup->IsComplete());
        reload(); QCOMPARE(setup->State().value("step").toInt(), 6); enableFails = false;
        QVERIFY(setup->Complete()); QVERIFY(setup->IsComplete()); QVERIFY(!setup->SetPrivacy(true, true)); QVERIFY(!setup->CreateUser("other", "Other", "password123"));
    }
    void failedPasswordCanBeRetriedAfterRestartWithoutTakingAnotherAccount() {
        QVERIFY(setup->SetLocale("en_US.UTF-8", "us", "UTC")); passwordFails = true;
        QVERIFY(!setup->CreateUser("maker", "Maker", "a-long-test-password")); QVERIFY(user);
        reload(); QCOMPARE(setup->State().value("step").toInt(), 2);
        QVERIFY(!setup->CreateUser("another", "Other", "a-long-test-password")); passwordFails = false;
        QVERIFY(setup->CreateUser("maker", "Maker", "a-long-test-password"));
        QFile state(root.path() + "/var/lib/fidan/setup-state.json"); QVERIFY(state.open(QIODevice::ReadOnly));
        const auto bytes = state.readAll(); QVERIFY(!bytes.contains("password")); QVERIFY(!bytes.contains("a-long-test"));
        QVERIFY(!(state.permissions() & (QFileDevice::ReadGroup | QFileDevice::ReadOther)));
        QCOMPARE(std::count_if(commands.cbegin(), commands.cend(), [](const QStringList &command) { return command.first().endsWith("useradd"); }), 1);
    }
    void refusesExistingAccountsAndCorruptState() {
        QVERIFY(setup->SetLocale("en_US.UTF-8", "us", "UTC")); user = true;
        QVERIFY(!setup->CreateUser("existing", "Existing", "a-long-test-password"));
        write("/var/lib/fidan/setup-state.json", "broken"); reload(); QVERIFY(setup->State().contains("error")); QVERIFY(!setup->SkipUpdates()); QVERIFY(!setup->Complete());
    }
    void partialAccountWithoutHomeIsRepairedBeforeItIsReady() {
        QVERIFY(setup->SetLocale("en_US.UTF-8", "us", "UTC")); homeFails = true;
        QVERIFY(!setup->CreateUser("maker", "Maker", "a-long-test-password")); QVERIFY(user);
        QVERIFY(!setup->State().value("accountReady").toBool());
        reload(); homeFails = false;
        QVERIFY(setup->CreateUser("maker", "Maker", "a-long-test-password"));
        QVERIFY(setup->State().value("accountReady").toBool());
        QCOMPARE(std::count_if(commands.cbegin(), commands.cend(), [](const QStringList &command) { return command.first().endsWith("useradd"); }), 1);
    }
    void completionRefusesAnUnavailableHomeEvenWithSavedReadyState() {
        account(); QVERIFY(setup->SkipNetwork()); QVERIFY(setup->SetPrivacy(true, false, true)); QVERIFY(setup->SkipUpdates());
        homeFails = true; QVERIFY(!setup->Complete()); QVERIFY(!setup->IsComplete());
        homeFails = false; QVERIFY(setup->Complete());
    }
    void keyboardAndNetworkAreValidated() {
        QVERIFY(!setup->SetLocale("evil", "us", "UTC")); QVERIFY(!setup->SetLocale("en_US.UTF-8", "us", "../../etc/passwd"));
        account(); QCOMPARE(setup->ScanWifi(), QStringList({"Home Wi-Fi", "Workshop"}));
        QVERIFY(!setup->ConnectWifi("net\nother", "secret")); QVERIFY(setup->ConnectWifi("Workshop", "wifi-secret"));
        QVERIFY(!commands.last().contains("hidden")); QVERIFY(setup->ConnectWifi("Hidden workshop", "wifi-secret")); QVERIFY(commands.last().contains("hidden"));
        QFile keyboard(root.path() + "/etc/default/keyboard"); QVERIFY(keyboard.open(QIODevice::ReadOnly)); QVERIFY(keyboard.readAll().contains("XKBLAYOUT=\"tr\""));
        QFile locale(root.path() + "/etc/locale.conf"); QVERIFY(locale.open(QIODevice::ReadOnly)); QCOMPARE(locale.readAll(), QByteArray("LANG=tr_TR.UTF-8\n"));
        QFile state(root.path() + "/var/lib/fidan/setup-state.json"); QVERIFY(state.open(QIODevice::ReadOnly)); QVERIFY(!state.readAll().contains("wifi-secret"));
    }
    void cannotConsentToUnimplementedDiagnostics() {
        account(); QVERIFY(!setup->SetPrivacy(false, true, false));
        QVERIFY(!setup->State().value("privacyDone").toBool());
    }
    void preferenceValidationAndFailedWriteKeepSetupRecoverable() {
        QVariantMap preferences{{"themeMode","light"},{"accentColor","#2589ff"},{"dockPosition","bottom"},{"panelPosition","top"},{"panelStyle","compact"},{"dateFormat","iso"},{"language","tr_TR.UTF-8"},{"keyboardLayout","tr"},{"reducedMotion",true}};
        QVERIFY(!setup->SetDesktopPreferences(preferences));account();
        auto invalid=preferences;invalid["command"]="untrusted";QVERIFY(!setup->SetDesktopPreferences(invalid));
        invalid=preferences;invalid["reducedMotion"]="true";QVERIFY(!setup->SetDesktopPreferences(invalid));
        invalid=preferences;invalid["accentColor"]="#2589ff\n";QVERIFY(!setup->SetDesktopPreferences(invalid));
        QVERIFY(setup->SetDesktopPreferences(preferences));reload();
        QVERIFY(setup->State().value("desktopPreferencesReady").toBool());
        QVERIFY(setup->SkipNetwork());QVERIFY(setup->SetPrivacy(true,false,true));QVERIFY(setup->SkipUpdates());
        profileFails=true;QVERIFY(!setup->Complete());QVERIFY(!setup->IsComplete());
        QVERIFY(std::none_of(commands.cbegin(),commands.cend(),[](const auto &args){return args.contains("--unit=fidan-firstboot-reboot");}));
        profileFails=false;QVERIFY(setup->Complete());
    }
    void nativeInstallerCanSetAllThreePrivacyOptionsOverDBus() {
        account();
        auto bus = QDBusConnection::sessionBus(); QVERIFY(bus.isConnected());
        QVERIFY(bus.registerService("org.fidan.FirstBoot1"));
        QVERIFY(bus.registerObject("/org/fidan/FirstBoot1", setup.get(), QDBusConnection::ExportAllSlots));
        FirstBootClient client(bus); QSignalSpy finished(&client, &FirstBootClient::operationFinished);
        QVERIFY(client.setPrivacy(true, false, true));
        QTRY_COMPARE(finished.count(), 1);
        QVERIFY(finished.first().at(1).toBool());
        bus.unregisterObject("/org/fidan/FirstBoot1"); bus.unregisterService("org.fidan.FirstBoot1");
        QVERIFY(setup->State().value("privacyDone").toBool());
        QVERIFY(setup->State().value("sshEnabled").toBool());
        QVERIFY(setup->State().value("remoteEnabled").toBool());
    }
    void clientAndWizardStayResponsiveAndResume_data() {
        QTest::addColumn<int>("width"); QTest::addColumn<int>("height");
        QTest::newRow("1280x720") << 1280 << 720; QTest::newRow("1920x1080") << 1920 << 1080; QTest::newRow("3840x2160") << 3840 << 2160;
    }
    void clientAndWizardStayResponsiveAndResume() {
        QFETCH(int, width); QFETCH(int, height);
        account();
        auto bus = QDBusConnection::sessionBus(); QVERIFY(bus.isConnected());
        QVERIFY(bus.registerService("org.fidan.FirstBoot1")); QVERIFY(bus.registerObject("/org/fidan/FirstBoot1", setup.get(), QDBusConnection::ExportAllSlots));
        FirstBootClient client(bus); QSignalSpy finished(&client, &FirstBootClient::operationFinished);
        QTranslator translator;
        const QString translation = qEnvironmentVariable("FIDAN_TEST_TRANSLATOR");
        if (!translation.isEmpty()) { QVERIFY(translator.load(translation)); QCoreApplication::installTranslator(&translator); }
        QQmlApplicationEngine engine; engine.rootContext()->setContextProperty("Setup", &client);
        QStringList warnings; connect(&engine, &QQmlEngine::warnings, this, [&](const QList<QQmlError> &errors) { for (const auto &e : errors) warnings.append(e.toString()); });
        engine.load(QUrl::fromLocalFile(QString(FIDAN_SOURCE_DIR) + "/src/firstboot/qml/FirstBoot.qml"));
        QVERIFY2(!engine.rootObjects().isEmpty(), qPrintable(warnings.join('\n')));
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window); window->showNormal(); window->resize(width, height);
        QTRY_COMPARE(window->property("step").toInt(), 3);
        client.scanWifi(); QTRY_VERIFY(!client.busy()); QCOMPARE(client.networks().size(), 2);
        QVERIFY(client.skipNetwork()); QTRY_COMPARE(finished.count(), 1); QTRY_COMPARE(window->property("step").toInt(), 4);
        QVERIFY(client.setPrivacy(false, false, true)); QTRY_COMPARE(finished.count(), 2); QTRY_COMPARE(window->property("step").toInt(), 5);
        QVERIFY(QFileInfo::exists(root.path() + "/etc/fidan/remote-enabled"));
        QVERIFY(client.skipUpdates()); QTRY_COMPARE(finished.count(), 3); QTRY_COMPARE(window->property("step").toInt(), 6);
        window->setProperty("step", 2);
        auto *password = window->findChild<QObject *>("setupPassword"); auto *confirmation = window->findChild<QObject *>("setupPasswordConfirmation");
        auto *next = window->findChild<QObject *>("setupContinue"); QVERIFY(password && confirmation && next);
        password->setProperty("text", "a-long-password"); confirmation->setProperty("text", "different-password"); QTRY_VERIFY(!next->property("enabled").toBool());
        confirmation->setProperty("text", "a-long-password"); QTRY_VERIFY(next->property("enabled").toBool());
        password->setProperty("text", ""); confirmation->setProperty("text", "");
        const QString output = qEnvironmentVariable("FIDAN_SETUP_CAPTURE_DIR");
        if (!output.isEmpty()) {
            QVERIFY(QDir().mkpath(output));
            for (int step = 0; step <= 6; ++step) { window->setProperty("step", step); QTest::qWait(80); QVERIFY(window->grabWindow().save(output + QString("/setup-%1-%2x%3.png").arg(step).arg(width).arg(height))); }
        }
        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join('\n')));
        window->close(); bus.unregisterObject("/org/fidan/FirstBoot1"); bus.unregisterService("org.fidan.FirstBoot1");
    }
    void nativeDesktopReview() {
        if (!qEnvironmentVariableIsSet("FIDAN_NATIVE_SETUP_REVIEW")) QSKIP("Opt-in real compositor review");
        QCOMPARE(QGuiApplication::platformName(), QString("wayland"));
        auto bus = QDBusConnection::sessionBus(); QVERIFY(bus.isConnected());
        QVERIFY(bus.registerService("org.fidan.FirstBoot1")); QVERIFY(bus.registerObject("/org/fidan/FirstBoot1", setup.get(), QDBusConnection::ExportAllSlots));
        FirstBootClient client(bus);
        QTranslator translator;
        QVERIFY(translator.load(qEnvironmentVariable("FIDAN_TEST_TRANSLATOR"))); QCoreApplication::installTranslator(&translator);
        QQmlApplicationEngine engine; engine.rootContext()->setContextProperty("Setup", &client);
        engine.load(QUrl::fromLocalFile(QString(FIDAN_SOURCE_DIR) + "/src/firstboot/qml/FirstBoot.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window);
        QTRY_VERIFY(window->isVisible()); QVERIFY(window->visibility() != QWindow::FullScreen);
        // Keep a real native window interactive for browser/RFB review. The
        // service above writes only into QTemporaryDir; all system runners are stubs.
        QTest::qWait(180000);
        window->close(); bus.unregisterObject("/org/fidan/FirstBoot1"); bus.unregisterService("org.fidan.FirstBoot1");
    }
    void unifiedDesktopWelcomeOwnsDeviceSetup() {
        auto bus=QDBusConnection::sessionBus();QVERIFY(bus.isConnected());
        QVERIFY(bus.registerService("org.fidan.FirstBoot1"));QVERIFY(bus.registerObject("/org/fidan/FirstBoot1",setup.get(),QDBusConnection::ExportAllSlots));
        FirstBootClient client(bus);
        QTranslator translator;
        const QString translation=qEnvironmentVariable("FIDAN_UNIFIED_TRANSLATOR");
        if(!translation.isEmpty()){QVERIFY(translator.load(translation));QCoreApplication::installTranslator(&translator);}
        QQmlApplicationEngine engine;
        QQmlComponent appearance(&engine);
        appearance.loadUrl(QUrl::fromLocalFile(QString(FIDAN_SOURCE_DIR)+"/tests/fixtures/UnifiedAppearance.qml"));
        std::unique_ptr<QObject> controller(appearance.create());QVERIFY2(controller,qPrintable(appearance.errorString()));
        engine.rootContext()->setContextProperty("Controller",controller.get());engine.rootContext()->setContextProperty("DeviceSetup",&client);
        QStringList warnings;connect(&engine,&QQmlEngine::warnings,this,[&](const auto &errors){for(const auto &e:errors)warnings.append(e.toString());});
        const QString directory=QString(FIDAN_SOURCE_DIR)+"/src/shell/qml/components";
        const QString wrapper="import QtQuick\nimport QtQuick.Controls\nimport \".\"\nApplicationWindow {width:1280;height:800;visible:true;Image {anchors.fill:parent;source:\"file://"+QString(FIDAN_SOURCE_DIR)+"/artwork/wallpapers/fidan-mountain-dawn.png\";fillMode:Image.PreserveAspectCrop} DeviceWelcomeWizard {anchors.centerIn:parent;width:920;height:700}}";
        engine.loadData(wrapper.toUtf8(),QUrl::fromLocalFile(directory+"/UnifiedTest.qml"));
        QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
        auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);
        auto *wizard=window->findChild<QObject*>("deviceWelcomeWizard");QVERIFY(wizard);
        auto click=[&](const char *name){auto *button=window->findChild<QObject*>(name);if(!button||!button->property("enabled").toBool())return false;return QMetaObject::invokeMethod(button,"clicked");};
        QTRY_VERIFY(!client.state().isEmpty());QCOMPARE(wizard->property("step").toInt(),0);
        const QString captures=qEnvironmentVariable("FIDAN_UNIFIED_CAPTURE_DIR");
        if(!captures.isEmpty()){QDir().mkpath(captures);QTest::qWait(200);QVERIFY(window->grabWindow().save(captures+"/unified-welcome.png"));}
        QVERIFY(click("deviceWelcomeNext"));QCOMPARE(wizard->property("step").toInt(),1);
        QVERIFY(click("deviceWelcomeNext"));QTRY_COMPARE(wizard->property("step").toInt(),2);
        for(const auto &entry:QList<QPair<QString,QString>>{{"deviceUsername","maker"},{"deviceDisplayName","Maker"},{"devicePassword","a-long-test-password"},{"devicePasswordConfirmation","a-long-test-password"}}){auto *field=window->findChild<QObject*>(entry.first);QVERIFY(field);field->setProperty("text",entry.second);}
        QVERIFY(click("deviceWelcomeNext"));QTRY_COMPARE(wizard->property("step").toInt(),3);QTRY_VERIFY(!client.busy());
        QVERIFY(click("deviceSkipNetwork"));QTRY_COMPARE(wizard->property("step").toInt(),4);
        controller->setProperty("themeMode","light");controller->setProperty("accentColor","#2589ff");
        if(!captures.isEmpty()){QTest::qWait(100);QVERIFY(window->grabWindow().save(captures+"/unified-appearance.png"));}
        QVERIFY(click("deviceWelcomeNext"));QTRY_COMPARE(wizard->property("step").toInt(),5);
        QVERIFY(setup->State().value("desktopPreferencesReady").toBool());
        QVERIFY(click("deviceWelcomeNext"));QTRY_COMPARE(wizard->property("step").toInt(),6);
        QVERIFY(click("deviceSkipUpdates"));QTRY_COMPARE(wizard->property("step").toInt(),7);
        QVERIFY(click("deviceWelcomeNext"));QTRY_VERIFY(setup->IsComplete());
        QVERIFY(profileInput.contains("\"themeMode\":\"light\""));QVERIFY(profileInput.contains("#2589ff"));QVERIFY(!profileInput.contains("password"));
        QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
        window->close();delete window;engine.clearComponentCache();
        QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
        bus.unregisterObject("/org/fidan/FirstBoot1");bus.unregisterService("org.fidan.FirstBoot1");
    }
};
QTEST_MAIN(FirstBootTest)
#include "tst_firstboot.moc"
