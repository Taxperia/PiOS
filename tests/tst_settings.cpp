// SPDX-License-Identifier: GPL-3.0-or-later
#include "SettingsData.h"
#include "ShellController.h"
#include "ProductInfo.h"
#include "SystemMonitor.h"
#include "UpdateController.h"
#include "DisplayController.h"
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

class SettingsTest final : public QObject {
    Q_OBJECT
    QTemporaryDir home;
    QByteArray originalPath;
    static void write(const QString &path, const QByteArray &bytes) {
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(bytes), bytes.size());
    }
    static QQuickItem *visual(QQuickItem *item, const QString &name) {
        if(item->objectName()==name) return item;
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
        const QStringList pages{"System","Personalization","Storage","Network","About","Bluetooth","Sound","Power","Keyboard & Mouse","Applications","Privacy","Display"};
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
};
QTEST_MAIN(SettingsTest)
#include "tst_settings.moc"
