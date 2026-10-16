// SPDX-License-Identifier: GPL-3.0-or-later
#include "SettingsData.h"
#include "ShellController.h"
#include "ProductInfo.h"
#include "SystemMonitor.h"
#include "UpdateController.h"
#include "DisplayController.h"
#include "DesktopSettings.h"
#include "ThemeIconProvider.h"
#include <QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QSettings>
#include <QStorageInfo>
#include <QSysInfo>
#include <QWheelEvent>
#include <QFile>
#include <QImage>
#include <QTime>
#include <QFileInfo>
#include <QScopeGuard>
#include <QJsonDocument>
#include <QJsonArray>
#include <memory>

class SettingsTest final : public QObject {
    Q_OBJECT
    QTemporaryDir home;
    QByteArray originalPath;
    static void write(const QString &path, const QByteArray &bytes) {
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(bytes), bytes.size());
    }
    static QQuickItem *visual(QQuickItem *item, const QString &name) {
        if(item->objectName()==name && item->isVisible()) return item;
        for(auto *child:item->childItems()) if(auto *found=visual(child,name)) return found;
        return nullptr;
    }
private slots:
    void initTestCase() {
        QVERIFY(home.isValid()); QStandardPaths::setTestModeEnabled(false);
        qputenv("XDG_CONFIG_HOME", home.path().toUtf8()); originalPath=qgetenv("PATH"); QQuickStyle::setStyle("Basic");
    }
    void cleanup() { qputenv("PATH", originalPath); QFile::remove(home.filePath("fidan/shell.conf")); }
    void appearancePersistsAcrossControllers() {
        ShellController first(true,true), second(true,true);
        first.setThemeMode("light"); first.setAccentColor("#2589ff"); first.setDockPosition("bottom"); first.setDockIconSize(40);
        QTRY_COMPARE_WITH_TIMEOUT(second.dockPosition(), "bottom", 5000);
        QCOMPARE(second.themeMode(), "light"); QVERIFY(!second.darkTheme()); QCOMPARE(second.accentColor(), "#2589ff"); QCOMPARE(second.dockIconSize(),40);
        first.setThemeMode("invalid"); first.setDockPosition("middle"); first.setAccentColor("transparent");
        QCOMPARE(first.themeMode(),"light"); QCOMPARE(first.dockPosition(),"bottom"); QCOMPARE(first.accentColor(),"#2589ff");
        first.setDockIconSize(1000); QCOMPARE(first.dockIconSize(),48);
        first.setThemeMode("auto"); QCOMPARE(first.themeMode(),"auto");
        QCOMPARE(first.darkTheme(),QTime::currentTime().hour()<8 || QTime::currentTime().hour()>=19);
        QImage image(40,20,QImage::Format_RGB32); image.fill(Qt::red); QVERIFY(image.save(home.filePath("lock photo.png")));
        first.setLockWallpaperUrl(QUrl::fromLocalFile(home.filePath("lock photo.png")).toString());
        QVERIFY(!first.lockWallpaperUrl().isEmpty()); const auto saved=first.lockWallpaperUrl(); first.setLockWallpaperUrl("https://invalid.example/photo.png"); QCOMPARE(first.lockWallpaperUrl(),saved);
    }
    void systemAndStorageAreReal() {
        SettingsData data(true);
        QCOMPARE(data.info().value("architecture").toString(),QSysInfo::currentCpuArchitecture());
        QCOMPARE(data.info().value("kernel").toString(),QSysInfo::kernelVersion());
        QVERIFY(!data.info().value("cpu").toString().isEmpty()); QVERIFY(!data.volumes().isEmpty());
        const auto root=data.volumes().first().toMap(); QStorageInfo storage("/");
        QCOMPARE(root.value("mount").toString(),"/"); QCOMPARE(root.value("total").toLongLong(),storage.bytesTotal());
        QVERIFY(root.value("percent").toDouble()>=0 && root.value("percent").toDouble()<=100);
        QVERIFY(data.wallpapers().size()>=8);
        for(const auto &item:data.wallpapers()) QVERIFY(QFileInfo::exists(item.toMap().value("url").toUrl().toLocalFile()));
        QVERIFY(!data.toolAvailable("unknown-tool"));
    }
    void loadsAndNavigatesAllPages() {
        ShellController controller(true,true); SettingsData data(true); ProductInfo product; SystemMonitor monitor(true); UpdateController updates; DisplayController displays(true);
        QQmlApplicationEngine engine; QStringList warnings;
        connect(&engine,&QQmlApplicationEngine::warnings,this,[&](const QList<QQmlError> &errors){ for(const auto &e:errors) warnings.append(e.toString()); });
        engine.addImageProvider("theme",new ThemeIconProvider);
        auto *context=engine.rootContext(); context->setContextProperty("Controller",&controller); context->setContextProperty("SettingsData",&data); context->setContextProperty("Product",&product); context->setContextProperty("SystemMonitor",&monitor); context->setContextProperty("Updates",&updates); context->setContextProperty("Displays",&displays);
        engine.loadData("import QtQuick\nimport QtQuick.Controls\nimport \".\"\nApplicationWindow { width:1160; height:650; visible:true; SettingsContent { objectName:'settings'; anchors.fill:parent } }",QUrl::fromLocalFile(QStringLiteral(FIDAN_SOURCE_DIR "/src/common/qml/test-settings.qml")));
        QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
        auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window); window->requestActivate(); QTest::qWait(100);
        auto *settings=window->findChild<QQuickItem *>("settings"); QVERIFY(settings);
        const QStringList pages{"System","Personalization","Storage","Network","About","Bluetooth","Sound","Power","Keyboard & Mouse","Applications","Privacy","Display",
            "System/date","System/region","System/users","System/backup","System/advanced","System/storageTools","System/licenses",
            "Network/Wi-Fi","Network/Ethernet","Network/VPN","Network/Advanced","Applications/Installed","Applications/Defaults","Applications/Detail/missing.desktop",
            "Privacy/Encryption","Privacy/Permissions","Privacy/Data","Privacy/Credentials"};
        for(const auto &page:pages) {
            settings->setProperty("selectedCategory",page); QTest::qWait(80);
            auto *loader=window->findChild<QQuickItem *>("settingsPageLoader"); QVERIFY(loader); QCOMPARE(loader->property("status").toInt(),1);
            QVERIFY2(warnings.isEmpty(),qPrintable(page+": "+warnings.join('\n')));
            if (page == "Keyboard & Mouse") {
                auto *combo=visual(window->contentItem(),"keyboard-layout"); QVERIFY(combo);
                for(int selection : {1,0,2,0}) {
                    QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,combo->mapToScene(QPointF(combo->width()/2,combo->height()/2)).toPoint());
                    QTest::qWait(80);
                    auto *option=visual(window->contentItem(),QStringLiteral("keyboard-layout-option-%1").arg(selection)); QVERIFY(option);
                    QCOMPARE(option->property("text").toString(),QStringList({"English (US)","Türkçe","Deutsch","English (UK)"}).at(selection));
                    QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,option->mapToScene(QPointF(option->width()/2,option->height()/2)).toPoint());
                    QCOMPARE(controller.keyboardLayout(),QStringList({"us","tr","de","gb"}).at(selection));
                    QVERIFY(window->isVisible());
                }
            }
        }
        settings->setProperty("selectedCategory","Network"); QTest::qWait(80);
        auto *ethernet=visual(window->contentItem(),"networkEthernet"); QVERIFY(ethernet);
        QVERIFY(QMetaObject::invokeMethod(ethernet,"triggered"));
        QCOMPARE(settings->property("selectedCategory").toString(),QString("Network/Ethernet"));
        auto *back=visual(window->contentItem(),"settingsBack"); QVERIFY(back); QVERIFY(back->isVisible());
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,back->mapToScene(QPointF(back->width()/2,back->height()/2)).toPoint());
        QCOMPARE(settings->property("selectedCategory").toString(),QString("Network"));
        settings->setProperty("selectedCategory","Personalization"); QTest::qWait(50);
        auto *light=visual(window->contentItem(),"theme-light"); QVERIFY(light);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,light->mapToScene(QPointF(light->width()/2,light->height()/2)).toPoint());
        QCOMPARE(controller.themeMode(),"light"); QVERIFY(!controller.darkTheme());
        for(const auto &page:pages) {
            settings->setProperty("selectedCategory",page); QTest::qWait(80);
            QVERIFY2(warnings.isEmpty(),qPrintable(page+": "+warnings.join('\n')));
        }
        settings->setProperty("selectedCategory","Personalization"); QTest::qWait(80);
        auto *scroll=window->findChild<QQuickItem *>("settingsScroll"); QVERIFY(scroll); auto *flick=scroll->property("contentItem").value<QQuickItem *>(); QVERIFY(flick);
        QWheelEvent wheel(QPointF(1000,450),QPointF(1000,450),QPoint(),QPoint(0,-120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false); QCoreApplication::sendEvent(window,&wheel);
        QTRY_VERIFY(flick->property("contentY").toReal()>0);
        window->setWidth(780); window->setHeight(500); QTest::qWait(100); QVERIFY(scroll->property("contentHeight").toReal()>scroll->height());
        QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n'))); window->close();
    }
    void readsEscapedNetworksAndRetriesFailedConnection() {
        QTemporaryDir tools;
        const QByteArray program=R"PY(#!/usr/bin/python3
import pathlib,sys
p=pathlib.Path(__file__).parent
a=sys.argv[1:]
if 'status' in a: print('wlan0:wifi:connected:Home\\:5G')
elif 'list' in a: print('*:Home\\:5G:86:5180 MHz:WPA2:866 Mbit/s\n:Guest:40:2412 MHz:--:54 Mbit/s')
elif 'show' in a: print('GENERAL.DEVICE:wlan0\nGENERAL.TYPE:wifi\nGENERAL.CONNECTION:Home\\:5G\nIP4.ADDRESS[1]:192.168.1.42/24\nIP4.GATEWAY:192.168.1.1\nIP4.DNS[1]:1.1.1.1')
elif 'connect' in a:
    (p/'connect-args').write_text('\n'.join(a))
    (p/'connect-input').write_text(sys.stdin.read())
    if (p/'reject').exists(): print('Wrong password',file=sys.stderr);sys.exit(10)
)PY";
        write(tools.filePath("nmcli"),program); QVERIFY(QFile::setPermissions(tools.filePath("nmcli"),QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner));
        qputenv("PATH",tools.path().toUtf8()+":"+originalPath);
        SettingsData data; data.refreshNetwork(true); QTRY_VERIFY(!data.networkBusy());
        QVERIFY2(data.networkError().isEmpty(),qPrintable(data.networkError())); QVERIFY(data.wifiAvailable()); QCOMPARE(data.networks().size(),2);
        QCOMPARE(data.networks().first().toMap().value("ssid").toString(),"Home:5G"); QCOMPARE(data.connection().value("address").toString(),"192.168.1.42/24");
        write(tools.filePath("reject"),"1"); data.connectNetwork("Home:5G","bad password"); QTRY_VERIFY(!data.networkBusy()); QVERIFY(data.networkError().contains("Wrong password"));
        QFile::remove(tools.filePath("reject")); data.connectNetwork("Home:5G","correct password"); QTRY_VERIFY(!data.networkBusy()); QVERIFY(data.networkError().isEmpty()); QCOMPARE(data.networks().size(),2);
        QFile arguments(tools.filePath("connect-args")); QVERIFY(arguments.open(QIODevice::ReadOnly));
        const auto invocation=arguments.readAll(); QVERIFY(invocation.contains("--ask")); QVERIFY(!invocation.contains("password"));
        QFile input(tools.filePath("connect-input")); QVERIFY(input.open(QIODevice::ReadOnly)); QCOMPARE(input.readAll(),QByteArray("correct password\n"));
        data.connectNetwork("Home:5G","injected\nsecond-line"); QVERIFY(!data.networkBusy()); QVERIFY(!data.networkError().isEmpty());
        QCOMPARE(SettingsData::splitFields("a\\:b:c\\\\d"),QStringList({"a:b","c\\d"}));
    }
    void closingSettingsStopsPendingUpdateCheckSafely() {
        QTemporaryDir tools;
        write(tools.filePath("fidan-update"),"#!/usr/bin/python3\nimport pathlib,time\npathlib.Path(__file__).with_name('started').touch()\ntime.sleep(30)\n");
        QVERIFY(QFile::setPermissions(tools.filePath("fidan-update"),QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner));
        qputenv("PATH",tools.path().toUtf8()+":"+originalPath);
        auto updates=std::make_unique<UpdateController>();updates->check();
        QTRY_VERIFY(QFileInfo::exists(tools.filePath("started")));QVERIFY(updates->busy());
        QElapsedTimer timer;timer.start();updates.reset();QVERIFY(timer.elapsed()<2500);
    }
    void desktopServicesUseRealRecordsAndBoundedCommands() {
        QTemporaryDir tools; QVERIFY(tools.isValid());
        const auto oldData=qgetenv("XDG_DATA_HOME"),oldDirs=qgetenv("XDG_DATA_DIRS");
        const auto restore=qScopeGuard([&]{qputenv("XDG_DATA_HOME",oldData);qputenv("XDG_DATA_DIRS",oldDirs);qputenv("PATH",originalPath);});
        QVERIFY(QDir().mkpath(tools.filePath("applications")));
        write(tools.filePath("applications/fixture.desktop"),"[Desktop Entry]\nType=Application\nName=Fixture App\nExec=fixture\nIcon=application-x-executable\n");
        write(tools.filePath("applications/hidden.desktop"),"[Desktop Entry]\nType=Application\nName=Hidden\nExec=hidden\nNoDisplay=true\n");
        const QByteArray fake=R"PY(#!/usr/bin/python3
import json,pathlib,sys
p=pathlib.Path(__file__).parent
tool=pathlib.Path(sys.argv[0]).name
a=sys.argv[1:]
def output(value): print(json.dumps(value))
def action():
    (p/'action').write_text(json.dumps([tool,*a]))
    if (p/'reject').exists(): print('Fixture refused change',file=sys.stderr);sys.exit(1)
if tool=='pactl':
    if a==['get-default-sink']: print('speaker')
    elif a==['get-default-source']: print('mic')
    elif a[:2]==['--format=json','list']:
        v={'front-left':{'value':32768},'front-right':{'value':65536}}
        if a[2]=='sinks': output([dict(name='speaker',description='Speakers',volume=v,mute=False,state='RUNNING',ports={'headphones':{'description':'Headphones','availability':'available'},'unplugged':{'availability':'not available'}},active_port='headphones')])
        elif a[2]=='sources': output([dict(name='mic',description='Microphone',volume=v,mute=True,ports=[]),dict(name='speaker.monitor',volume=v)])
        else: output([dict(index=42,properties={'application.name':'Music'},volume=v)])
    else: action()
elif tool=='nmcli':
    if 'CONNECTIVITY' in a: print('full')
    elif 'RUNNING' in a: print('running')
    elif a[-2:]==['connection','show']: print(r'Office\:VPN:1234:vpn:tun0'+'\nEthernet:5678:802-3-ethernet:eth0')
    else: action()
elif tool=='powerprofilesctl':
    if a==['list']: print('  power-saver:\n* balanced:')
    elif a==['get']: print('balanced')
    else: action()
elif tool=='dpkg-query':
    if a[0]=='-W': print('fixture\t2.0\t128\tinstalled\nlibrary\t1.0\t16\tinstalled')
    else: print('fixture: '+str(p/'applications/fixture.desktop'))
elif tool=='flatpak':
    if a[0]=='list': print('org.test.App\tSandbox App\t3.0\tuser\t10 MB')
    elif a[0]=='info': print('[Context]\nshared=network;\nsockets=pulseaudio;\ndevices=all;\nfilesystems=home;!host;')
    else: action()
elif tool=='lsblk': output({'blockdevices':[{'name':'disk','type':'disk','children':[{'name':'secure','type':'crypt','mountpoints':['/'],'fstype':'ext4'}]}]})
elif tool=='fidan-input': action()
)PY";
        for(const auto &name:{"pactl","nmcli","powerprofilesctl","dpkg-query","flatpak","lsblk","fidan-input"}) {
            write(tools.filePath(name),fake); QVERIFY(QFile::setPermissions(tools.filePath(name),QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner));
        }
        qputenv("PATH",tools.path().toUtf8()+":"+originalPath); qputenv("XDG_DATA_HOME",tools.path().toUtf8());qputenv("XDG_DATA_DIRS",tools.path().toUtf8());
        SettingsData data; auto &hub=*qobject_cast<DesktopSettings *>(data.desktop()); hub.refresh(); QTRY_VERIFY_WITH_TIMEOUT(!hub.refreshing(),15000);
        const auto state=hub.state();
        QCOMPARE(state.value("connectivity").toString(),QString("full"));
        QCOMPARE(state.value("outputs").toList().size(),1); QCOMPARE(state.value("inputs").toList().size(),1);
        QCOMPARE(state.value("outputs").toList()[0].toMap().value("volume").toInt(),75);
        QVERIFY(state.value("outputs").toList()[0].toMap().value("default").toBool());
        QVERIFY(state.value("inputs").toList()[0].toMap().value("muted").toBool());
        QCOMPARE(state.value("vpns").toList().size(),1); QCOMPARE(state.value("vpns").toList()[0].toMap().value("name").toString(),QString("Office:VPN"));
        QCOMPARE(state.value("encryption").toString(),QString("encrypted"));
        QCOMPARE(state.value("applications").toList().size(),2);
        auto app=state.value("applications").toList()[0].toMap(); QCOMPARE(app.value("package").toString(),QString("fixture")); QCOMPARE(app.value("size").toLongLong(),131072);
        QCOMPARE(hub.permissions("org.test.App").value("network").toBool(),true);
        QCOMPARE(hub.permissions("org.test.App").value("filesystemSummary").toString(),QString("home"));
        {
            ShellController controller(true,true); ProductInfo product; SystemMonitor monitor(true); UpdateController updates; DisplayController displays(true);
            QQmlApplicationEngine engine; QStringList warnings;
            connect(&engine,&QQmlApplicationEngine::warnings,this,[&](const QList<QQmlError> &errors){for(const auto &e:errors)warnings.append(e.toString());});
            engine.addImageProvider("theme",new ThemeIconProvider);
            auto *context=engine.rootContext();context->setContextProperty("Controller",&controller);context->setContextProperty("SettingsData",&data);context->setContextProperty("Product",&product);context->setContextProperty("SystemMonitor",&monitor);context->setContextProperty("Updates",&updates);context->setContextProperty("Displays",&displays);
            engine.loadData("import QtQuick\nimport QtQuick.Controls\nimport \".\"\nApplicationWindow { width:1060; height:720; visible:true; SettingsContent { objectName:'settings'; anchors.fill:parent } }",QUrl::fromLocalFile(QStringLiteral(FIDAN_SOURCE_DIR "/src/common/qml/test-settings.qml")));
            QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
            auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first());auto *settings=window->findChild<QQuickItem *>("settings");QVERIFY(settings);
            for(const auto &page:{"Sound","Power","Network/VPN","Applications/Installed","Applications/Detail/fixture.desktop","Privacy/Permissions"}) {
                settings->setProperty("selectedCategory",page);QTest::qWait(100);QVERIFY2(warnings.isEmpty(),qPrintable(QString(page)+": "+warnings.join('\n')));
                if(QString(page)=="Privacy/Permissions") {auto *selector=visual(window->contentItem(),"privacyAppSelector");QVERIFY(selector);QCOMPARE(selector->property("count").toInt(),1);}
            }
            if(qEnvironmentVariableIsSet("FIDAN_SETTINGS_TEST_CAPTURE")) {
                settings->setProperty("selectedCategory","Sound");QTest::qWait(100);QVERIFY(window->grabWindow().save(qEnvironmentVariable("FIDAN_SETTINGS_TEST_CAPTURE")));
            }
            window->close();
        }
        QTRY_VERIFY_WITH_TIMEOUT(!hub.refreshing(),15000);
        const auto invocation=[&] {QFile file(tools.filePath("action"));if(!file.open(QIODevice::ReadOnly))return QJsonArray();return QJsonDocument::fromJson(file.readAll()).array();};
        const auto wait=[&] {QTRY_VERIFY_WITH_TIMEOUT(!hub.busy() && !hub.refreshing(),15000);QVERIFY2(hub.error().isEmpty(),qPrintable(hub.error()));};
        hub.setAudioVolume("sink","speaker",40); wait(); QCOMPARE(invocation(),QJsonArray({"pactl","set-sink-volume","speaker","40%"}));
        const auto previous=invocation(); hub.setAudioVolume("sink","unknown",50); hub.setAudioVolume("sink","speaker",101); hub.setPowerProfile("performance"); hub.activateConnection("unknown",true); hub.setAudioPort("sink","speaker","unplugged"); hub.setFlatpakPermission("org.unknown.App","network",false); hub.setPortalPermission("org.test.App","unknown","yes");
        QCOMPARE(invocation(),previous);
        hub.setAudioDefault("source","mic");wait();QCOMPARE(invocation(),QJsonArray({"pactl","set-default-source","mic"}));
        hub.setAudioMute("source","mic",true);wait();QCOMPARE(invocation(),QJsonArray({"pactl","set-source-mute","mic","1"}));
        hub.setAudioPort("sink","speaker","headphones");wait();QCOMPARE(invocation(),QJsonArray({"pactl","set-sink-port","speaker","headphones"}));
        hub.setApplicationVolume(42,25);wait();QCOMPARE(invocation(),QJsonArray({"pactl","set-sink-input-volume","42","25%"}));
        hub.activateConnection("1234",true);wait();QCOMPARE(invocation(),QJsonArray({"nmcli","--wait","15","connection","up","uuid","1234"}));
        hub.setPowerProfile("power-saver");wait();QCOMPARE(invocation(),QJsonArray({"powerprofilesctl","set","power-saver"}));
        hub.setFlatpakPermission("org.test.App","network",false);wait();QCOMPARE(invocation(),QJsonArray({"flatpak","override","--user","--unshare=network","org.test.App"}));
        hub.setFlatpakPermission("org.test.App","files",false);wait();QCOMPARE(invocation(),QJsonArray({"flatpak","override","--user","--nofilesystem=host:reset","org.test.App"}));
        hub.setCameraPermission("org.test.App","no");wait();QCOMPARE(invocation(),QJsonArray({"flatpak","permission-set","devices","camera","org.test.App","no"}));
        hub.setPortalPermission("org.test.App","location","no");wait();QCOMPARE(invocation(),QJsonArray({"flatpak","permission-set","location","location","org.test.App","NONE","0"}));
        hub.setInput("repeatRate",37);wait();QCOMPARE(hub.input().value("repeatRate").toInt(),37);QCOMPARE(invocation(),QJsonArray({"fidan-input","apply"}));
        hub.setInput("repeatRate",0); QCOMPARE(hub.input().value("repeatRate").toInt(),37);
        write(tools.filePath("reject"),"1");hub.setAudioMute("source","mic",false);QTRY_VERIFY_WITH_TIMEOUT(!hub.busy()&&!hub.refreshing(),15000);QVERIFY(hub.error().contains("Fixture refused"));
    }
};
QTEST_MAIN(SettingsTest)
#include "tst_settings.moc"
