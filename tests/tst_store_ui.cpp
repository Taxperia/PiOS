// SPDX-License-Identifier: GPL-3.0-or-later
#include "StoreModel.h"
#include "AppStyle.h"
#include "ThemeIconProvider.h"
#include <QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QFile>
#include <QSettings>

class StoreUiTest final : public QObject {
    Q_OBJECT
    QTemporaryDir tools;
    QByteArray originalPath;
    void write(const QString &name,const QByteArray &data) {
        QFile file(tools.filePath(name)); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(data),data.size()); file.close();
        QVERIFY(file.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner));
    }
    static QQuickItem *visual(QQuickItem *item,const QString &name) {
        if(item->objectName()==name) return item;
        for(auto *child:item->childItems()) if(auto *found=visual(child,name)) return found;
        return nullptr;
    }
private slots:
    void initTestCase() {
        QVERIFY(tools.isValid()); originalPath=qgetenv("PATH");
        qputenv("PATH",tools.path().toUtf8()+":"+originalPath);
        qputenv("XDG_CONFIG_HOME",tools.path().toUtf8());
        qputenv("FIDAN_STORE_CATALOG",FIDAN_SOURCE_DIR "/data/store/catalog.json");
        QQuickStyle::setStyle("Basic");
        const QByteArray flatpak=R"PY(#!/usr/bin/python3
import pathlib,sys,time
p=pathlib.Path(__file__).parent
a=sys.argv[1:]; command=a[0]
identifier=a[-1]; state=p/(identifier+'.installed')
if command=='list':
    for item in p.glob('*.installed'): print(item.name[:-10])
elif command=='info': sys.exit(0 if state.exists() else 1)
elif command=='remote-info': print('Version: 4.2\nArch: x86_64\nDownload: 42 MB\nInstalled: 128 MB\nLicense: GPL-3.0-or-later')
elif command=='remote-add': time.sleep(.1)
elif command=='install':
    for percent in (13,57,100):
        print('\x1b[?25l\x1b[2KDownloading '+str(percent)+'%',flush=True); time.sleep(.2)
    state.touch()
elif command=='uninstall': state.unlink(missing_ok=True)
)PY";
        write("flatpak",flatpak);
        write("apt","#!/bin/sh\nexit 0\n");
        write("dpkg-query","#!/bin/sh\nexit 1\n");
        write("apt-cache","#!/bin/sh\nprintf 'Version: 2.1\\nSize: 1024\\nInstalled-Size: 64\\nArchitecture: amd64\\n'\n");
    }
    void cleanupTestCase() { qputenv("PATH",originalPath); }
    void detailsSurviveFilteringAndVerifyProgress() {
        StoreModel model; QTRY_VERIFY(!model.busy()); QVERIFY(model.catalogSize()>=40);
        model.setFilter("GIMP"); QCOMPARE(model.rowCount(),1); model.selectEntry(0);
        QTRY_VERIFY(!model.selectedApp().value("loading").toBool());
        QCOMPARE(model.selectedApp().value("version").toString(),"4.2");
        QCOMPARE(model.selectedApp().value("downloadSize").toString(),"42 MB");
        QVERIFY(model.selectedApp().value("features").toStringList().size()>=3);
        model.setFilter("Krita"); QCOMPARE(model.rowCount(),1);
        QCOMPARE(model.selectedApp().value("identifier").toString(),"org.gimp.GIMP");
        model.installSelected(); QTRY_VERIFY(model.busy());
        QTRY_VERIFY_WITH_TIMEOUT(model.progress()==13 || model.progress()==57,3000);
        QCOMPARE(model.operationIdentifier(),"org.gimp.GIMP");
        QVERIFY(!model.selectedApp().value("installed").toBool());
        QTRY_VERIFY_WITH_TIMEOUT(!model.busy(),5000);
        QVERIFY2(model.errorText().isEmpty(),qPrintable(model.errorText()));
        QVERIFY(model.selectedApp().value("installed").toBool()); QCOMPARE(model.progress(),100);
        QVERIFY(!model.operationOutput().contains(QChar(0x1b)));
        QVERIFY(QFile::exists(tools.filePath("org.gimp.GIMP.installed")));
        QVERIFY(!QFile::exists(tools.filePath("org.kde.krita.installed")));
        model.removeApplication("org.gimp.GIMP"); QTRY_VERIFY(!model.busy());
        QVERIFY(!model.selectedApp().value("installed").toBool());
        QVERIFY(!QFile::exists(tools.filePath("org.gimp.GIMP.installed")));
    }
    void opensDetailPageAndReturnsToSearch() {
        StoreModel model; AppStyle style; QTRY_VERIFY(!model.busy());
        QQmlApplicationEngine engine; QStringList warnings;
        connect(&engine,&QQmlApplicationEngine::warnings,this,[&warnings](const QList<QQmlError> &errors){for(const auto &error:errors) warnings.append(error.toString());});
        engine.addImageProvider("theme",new ThemeIconProvider);
        engine.rootContext()->setContextProperty("storeBackend",&model);
        engine.rootContext()->setContextProperty("UiStyle",&style);
        // Match the installed QML module's sibling component layout.
        QTemporaryDir qml;
        QVERIFY(qml.isValid());
        QVERIFY(QFile::copy(QStringLiteral(FIDAN_SOURCE_DIR "/src/store/qml/Store.qml"),qml.filePath("Store.qml")));
        QVERIFY(QFile::copy(QStringLiteral(FIDAN_SOURCE_DIR "/src/shell/qml/components/PiBoard.qml"),qml.filePath("PiBoard.qml")));
        engine.load(QUrl::fromLocalFile(qml.filePath("Store.qml")));
        QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
        auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window); window->requestActivate(); QTest::qWait(100);
        auto *search=visual(window->contentItem(),"store-search"); QVERIFY(search); search->setProperty("text","GIMP");
        QTRY_COMPARE(model.rowCount(),1); QTest::qWait(100);
        auto *card=visual(window->contentItem(),"app-card-0"); QVERIFY(card);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,card->mapToScene(QPointF(card->width()/2,card->height()/2)).toPoint());
        QTRY_COMPARE(model.selectedApp().value("name").toString(),"GIMP"); QTest::qWait(100);
        auto *name=visual(window->contentItem(),"detail-app-name"); QVERIFY(name); QCOMPARE(name->property("text").toString(),"GIMP");
        auto *install=visual(window->contentItem(),"detail-primary-action"); QVERIFY(install);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,install->mapToScene(QPointF(install->width()/2,install->height()/2)).toPoint());
        QTRY_VERIFY_WITH_TIMEOUT(!model.busy(),5000); QVERIFY(model.selectedApp().value("installed").toBool());
        auto *remove=visual(window->contentItem(),"detail-remove"); QVERIFY(remove);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,remove->mapToScene(QPointF(remove->width()/2,remove->height()/2)).toPoint()); QTest::qWait(80);
        auto *cancel=visual(window->contentItem(),"remove-cancel"); QVERIFY(cancel);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,cancel->mapToScene(QPointF(cancel->width()/2,cancel->height()/2)).toPoint()); QVERIFY(model.selectedApp().value("installed").toBool());
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,remove->mapToScene(QPointF(remove->width()/2,remove->height()/2)).toPoint()); QTest::qWait(80);
        auto *confirm=visual(window->contentItem(),"remove-confirm"); QVERIFY(confirm);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,confirm->mapToScene(QPointF(confirm->width()/2,confirm->height()/2)).toPoint());
        QTRY_VERIFY(!model.busy()); QVERIFY(!model.selectedApp().value("installed").toBool());
        auto *back=visual(window->contentItem(),"store-back"); QVERIFY(back);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,back->mapToScene(QPointF(back->width()/2,back->height()/2)).toPoint());
        QVERIFY(model.selectedApp().isEmpty()); QCOMPARE(model.filter(),"GIMP");
        window->setWidth(860); window->setHeight(540); QTest::qWait(100);
        QSettings preferences(tools.filePath("fidan/shell.conf"),QSettings::IniFormat);
        preferences.setValue("appearance/darkTheme",false); preferences.sync();
        QTRY_VERIFY(!style.dark()); model.selectEntry(0); QTest::qWait(100);
        QVERIFY(visual(window->contentItem(),"detail-app-name")); model.clearSelection();
        QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
        window->close();
    }
    void readsAptDownloadAndPackageStatusPercentages() {
        write("pkexec","#!/bin/sh\nexec \"$@\"\n");
        write("apt-get",R"PY(#!/usr/bin/python3
import pathlib,time
p=pathlib.Path(__file__).parent
print('dlstatus:1:25.0000:Retrieving package',flush=True); time.sleep(.25)
print('pmstatus:dpkg:65.0000:Installing package',flush=True); time.sleep(.25)
(p/'apt-qterminal.installed').touch()
)PY");
        write("dpkg-query",R"PY(#!/usr/bin/python3
import pathlib,sys
if sys.argv[-1]=='qterminal' and (pathlib.Path(__file__).parent/'apt-qterminal.installed').exists(): print('ii ')
)PY");
        StoreModel model; QTRY_VERIFY(!model.busy()); model.setFilter("QTerminal"); QCOMPARE(model.rowCount(),1); model.selectEntry(0);
        QTRY_VERIFY(!model.selectedApp().value("loading").toBool()); QCOMPARE(model.selectedApp().value("version").toString(),"2.1");
        model.installSelected(); QTRY_COMPARE_WITH_TIMEOUT(model.progress(),25,3000); QCOMPARE(model.phaseText(),"İndiriliyor");
        QTRY_COMPARE_WITH_TIMEOUT(model.progress(),65,3000); QCOMPARE(model.phaseText(),"Kuruluyor");
        QTRY_VERIFY(!model.busy()); QVERIFY2(model.errorText().isEmpty(),qPrintable(model.errorText()));
        QVERIFY(model.selectedApp().value("installed").toBool()); QCOMPARE(model.progress(),100);
    }
};
QTEST_MAIN(StoreUiTest)
#include "tst_store_ui.moc"
