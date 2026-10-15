// SPDX-License-Identifier: GPL-3.0-or-later
#include "PhotoModel.h"
#include "DisplayController.h"
#include "AppStyle.h"
#include "ThemeIconProvider.h"
#include <QtTest>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QWheelEvent>
#include <QCryptographicHash>
#include <QFile>
#include <QImageWriter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <memory>

class PhotosDisplayTest : public QObject {
    Q_OBJECT
    QTemporaryDir sandbox;
    QByteArray oldPath;
    static void write(const QString &path, const QByteArray &bytes) {
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(bytes), bytes.size());
    }
    QByteArray commands() const { QFile file(sandbox.filePath("commands")); if (!file.open(QIODevice::ReadOnly)) return {}; return file.readAll(); }
    std::unique_ptr<DisplayController> display() {
        auto result = std::make_unique<DisplayController>();
        QTest::qWait(100);
        return result;
    }
private slots:
    void initTestCase() {
        QVERIFY(sandbox.isValid()); QQuickStyle::setStyle("Basic");
        // Isolate profiles using XDG_CONFIG_HOME; Qt test mode overrides that path.
        QStandardPaths::setTestModeEnabled(false);
        oldPath = qgetenv("PATH");
        qputenv("XDG_CONFIG_HOME", sandbox.filePath("config").toUtf8());
        qputenv("PATH", sandbox.path().toUtf8() + ":" + oldPath);
        // A deterministic compositor fixture. Every command is recorded; no real output is changed.
        const QByteArray script = "#!/usr/bin/python3\nimport sys,json,pathlib\np=pathlib.Path(__file__).parent\na=sys.argv[1:]\nwith (p/'commands').open('a') as f: f.write(json.dumps(a)+'\\n')\nif '--json' in a:\n print(json.dumps([{'name':'HDMI-A-1','description':'Test monitor','enabled':True,'scale':1,'transform':'normal','modes':[{'width':1920,'height':1080,'refresh':60,'current':True,'preferred':True},{'width':1280,'height':720,'refresh':60}]}]))\nelif '--dryrun' in a and (p/'reject').exists():\n print('Rejected fixture mode',file=sys.stderr);sys.exit(1)\n";
        write(sandbox.filePath("wlr-randr"), script);
        QVERIFY(QFile::setPermissions(sandbox.filePath("wlr-randr"), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    }
    void cleanup() {
        QFile::remove(sandbox.filePath("commands")); QFile::remove(sandbox.filePath("reject"));
        QFile::remove(sandbox.filePath("config/fidan/displays.json"));
    }
    void photosNavigateRotateAndCopy() {
        QTemporaryDir dir; QImage image(40, 20, QImage::Format_RGB32); image.fill(Qt::red);
        const QString first = dir.filePath(QString::fromUtf8("01 Türkçe photo.png"));
        QVERIFY(image.save(first)); QVERIFY(image.save(dir.filePath("02 next.jpg")));
        write(dir.filePath("note.txt"), "not a photo");
        QFile source(first); QVERIFY(source.open(QIODevice::ReadOnly)); const auto before = source.readAll(); source.close();
        PhotoModel photos; QVERIFY(photos.open(QUrl::fromLocalFile(first)));
        QCOMPARE(photos.count(), 2); QCOMPARE(photos.position(), 1); QCOMPARE(photos.pixelWidth(), 40);
        photos.previous(); QCOMPARE(photos.position(), 1);
        photos.rotate(1); QCOMPARE(photos.rotation(), 90);
        photos.copyImage(); QTRY_VERIFY_WITH_TIMEOUT(!photos.copying(), 3000);
        QCOMPARE(QGuiApplication::clipboard()->image().size(), QSize(20, 40));
        QVERIFY(source.open(QIODevice::ReadOnly)); QCOMPARE(source.readAll(), before);
        photos.next(); QCOMPARE(photos.position(), 2); QCOMPARE(photos.rotation(), 0);
        photos.next(); QCOMPARE(photos.position(), 2); photos.previous(); QCOMPARE(photos.position(), 1);
        QVERIFY(!photos.open(QUrl("https://example.invalid/photo.png"))); QVERIFY(!photos.errorText().isEmpty());
        QVERIFY(!photos.open(QUrl::fromLocalFile(dir.filePath("note.txt")))); QVERIFY(photos.url().isEmpty());
    }
    void exifOrientationIsApplied() {
        QTemporaryDir dir; const QString path = dir.filePath("rotated.jpg");
        QImage image(48, 24, QImage::Format_RGB32); image.fill(Qt::blue);
        QImageWriter writer(path, "jpeg"); writer.setTransformation(QImageIOHandler::TransformationRotate90);
        QVERIFY(writer.write(image));
        PhotoModel photos; QVERIFY(photos.open(QUrl::fromLocalFile(path)));
        QCOMPARE(QSize(photos.pixelWidth(), photos.pixelHeight()), QSize(24, 48));
        photos.copyImage(); QTRY_VERIFY(!photos.copying());
        QCOMPARE(QGuiApplication::clipboard()->image().size(), QSize(24, 48));
    }
    void photoWindowKeyboardAndDrop() {
        QTemporaryDir dir; QImage image(80, 40, QImage::Format_RGB32); image.fill(Qt::green);
        const QString first = dir.filePath("first.png"), second = dir.filePath("second.png");
        QVERIFY(image.save(first)); QVERIFY(image.save(second));
        PhotoModel photos; AppStyle style; QQmlApplicationEngine engine;
        engine.addImageProvider("theme", new ThemeIconProvider);
        engine.rootContext()->setContextProperty("PhotoLibrary", &photos); engine.rootContext()->setContextProperty("UiStyle", &style);
        engine.load(QUrl::fromLocalFile(QStringLiteral(FIDAN_SOURCE_DIR "/src/photos/qml/Photos.qml")));
        QVERIFY(!engine.rootObjects().isEmpty()); auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window);
        QVERIFY(photos.open(QUrl::fromLocalFile(first))); window->requestActivate(); QTest::qWait(100);
        auto *picture = window->findChild<QQuickItem *>("photoImage"); QVERIFY(picture);
        QTRY_COMPARE(picture->property("status").toInt(), 1);
        QTest::keyClick(window, Qt::Key_R); QTRY_COMPARE(photos.rotation(), 90);
        QTest::keyClick(window, Qt::Key_Right); QTRY_COMPARE(photos.position(), 2);
        QTest::keyClick(window, Qt::Key_Equal); QVERIFY(!window->property("fitMode").toBool());
        QTest::keyClick(window, Qt::Key_0); QVERIFY(window->property("fitMode").toBool());
        QMimeData mime; mime.setUrls({QUrl::fromLocalFile(first)});
        const QPoint point(window->width()/2, window->height()/2);
        QDragEnterEvent enter(point, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier); QCoreApplication::sendEvent(window, &enter);
        QDropEvent drop(point, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier); QCoreApplication::sendEvent(window, &drop);
        QTRY_COMPARE(photos.position(), 1);
        window->close();
    }
    void displayRejectsUnadvertisedModes() {
        auto controller = display(); QTRY_VERIFY(controller->available()); QCOMPARE(controller->selectedName(), "HDMI-A-1");
        controller->apply(999, 777, 60, 1, "normal"); QVERIFY(!controller->errorText().isEmpty());
        QVERIFY(!commands().contains("--dryrun"));
        controller->apply(1280, 720, 60, 0, "normal"); QVERIFY(!commands().contains("--dryrun"));
        write(sandbox.filePath("reject"), "1"); controller->apply(1280, 720, 60, 1, "normal");
        QTRY_VERIFY(!controller->busy()); QVERIFY(controller->errorText().contains("Rejected"));
        QVERIFY(!controller->confirmationPending()); QCOMPARE(commands().count("--output"), 1);
    }
    void displayKeepAndRevert() {
        auto controller = display(); QTRY_VERIFY(controller->available());
        controller->apply(1280, 720, 60, 1.25, "90"); QTRY_VERIFY(controller->confirmationPending());
        QVERIFY(commands().contains("--dryrun")); QVERIFY(controller->secondsRemaining() > 0);
        controller->revert(); QTRY_VERIFY(!controller->busy());
        QVERIFY(!controller->confirmationPending()); QVERIFY(commands().contains("1920x1080@60.000Hz"));
        controller->apply(1280, 720, 60, 1, "normal"); QTRY_VERIFY(controller->confirmationPending()); controller->keep();
        QTRY_VERIFY(!controller->busy());
        QFile profile(sandbox.filePath("config/fidan/displays.json")); QVERIFY(profile.open(QIODevice::ReadOnly));
        QCOMPARE(QJsonDocument::fromJson(profile.readAll()).object().value("HDMI-A-1").toObject().value("width").toInt(), 1280);
        QVERIFY(!commands().contains("--off"));
    }
    void displayAutomaticallyReverts() {
        auto controller = display(); QTRY_VERIFY(controller->available());
        controller->apply(1280, 720, 60, 1, "normal"); QTRY_VERIFY(controller->confirmationPending());
        QTRY_VERIFY_WITH_TIMEOUT(!controller->confirmationPending(), 17000); QTRY_VERIFY(!controller->busy());
        QVERIFY(commands().contains("1920x1080@60.000Hz"));
    }
    void displayClosingRestoresUnconfirmedMode() {
        auto controller = display(); QTRY_VERIFY(controller->available());
        controller->apply(1280, 720, 60, 1, "normal"); QTRY_VERIFY(controller->confirmationPending());
        controller.reset(); QVERIFY(commands().contains("1920x1080@60.000Hz"));
    }
    void displayPageAppliesAndConfirms() {
        auto controller = display(); QTRY_VERIFY(controller->available());
        QQmlApplicationEngine engine; engine.addImageProvider("theme", new ThemeIconProvider);
        engine.rootContext()->setContextProperty("Displays", controller.get());
        engine.rootContext()->setContextProperty("Controller", QVariantMap{{"darkTheme",true}, {"wallpaperUrl",QString()}, {"brightnessAvailable",false}, {"brightness",75}, {"interfaceScale",1.0}});
        engine.rootContext()->setContextProperty("Product", QVariantMap{{"wallpaperUrl",QString()}});
        engine.loadData("import QtQuick\nimport QtQuick.Controls\nimport QtQuick.Layouts\nimport \".\"\nApplicationWindow { width: 1100; height: 500; visible: true; SettingsScrollView { id: scroll; objectName: 'scroll'; anchors.fill: parent; contentWidth: availableWidth; ColumnLayout { width: scroll.availableWidth; Text { text: 'Display' } DisplayPage { objectName: 'page'; Layout.fillWidth: true } } } }", QUrl::fromLocalFile(QStringLiteral(FIDAN_SOURCE_DIR "/src/common/qml/test-display.qml")));
        QVERIFY(!engine.rootObjects().isEmpty()); auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window);
        auto *page = window->findChild<QQuickItem *>("page"); QVERIFY(page);
        auto *scroll = window->findChild<QQuickItem *>("scroll"); QVERIFY(scroll);
        QTRY_VERIFY(scroll->property("contentHeight").toReal() > scroll->height());
        auto *flick = scroll->property("contentItem").value<QQuickItem *>(); QVERIFY(flick);
        QWheelEvent wheel(QPointF(1000, 300), QPointF(1000, 300), QPoint(), QPoint(0,-120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(window, &wheel);
        QTRY_VERIFY(flick->property("contentY").toReal() > 0);
        auto *selector = window->findChild<QQuickItem *>("displaySelector"); QVERIFY(selector); QCOMPARE(selector->property("count").toInt(), 1);
        QVERIFY(!page->property("dirty").toBool());
        page->setProperty("draftScale", 1.25); QVERIFY(page->property("dirty").toBool());
        auto *apply = window->findChild<QQuickItem *>("displayApply"); QVERIFY(apply);
        QVERIFY(QMetaObject::invokeMethod(apply, "clicked")); QTRY_VERIFY(controller->confirmationPending());
        auto *dialog = window->findChild<QObject *>("displayConfirmation"); QVERIFY(dialog); QTRY_VERIFY(dialog->property("visible").toBool());
        controller->revert(); QTRY_VERIFY(!controller->busy()); QTRY_VERIFY(!dialog->property("visible").toBool());
        QVERIFY(!page->property("dirty").toBool()); window->close();
    }
    void cleanupTestCase() { qputenv("PATH", oldPath); }
};
QTEST_MAIN(PhotosDisplayTest)
#include "tst_photos_display.moc"
