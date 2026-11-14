// SPDX-License-Identifier: GPL-3.0-or-later
#include "AppModel.h"
#include "ShellController.h"
#include "NotificationService.h"
#include "DesktopModel.h"
#include "ThemeIconProvider.h"
#include <QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QSettings>
#include <QXmlStreamWriter>
#include <QDesktopServices>
#include <QJSValue>
#include <QScopeGuard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTranslator>
#include <algorithm>
#include <memory>

class LauncherUrlProbe final : public QObject {
    Q_OBJECT
public:
    QUrl opened;
public slots:
    void capture(const QUrl &url) { opened = url; }
};

class LauncherUiTest final : public QObject {
    Q_OBJECT
    // Keep the embedded QML fixture out of moc's C++ parser.
#ifndef Q_MOC_RUN
    QTemporaryDir home;
    QStringList warnings;
    std::unique_ptr<AppModel> applications;
    std::unique_ptr<ShellController> controller;
    std::unique_ptr<DesktopModel> desktop;
    std::unique_ptr<NotificationService> notifications;
    std::unique_ptr<QQmlApplicationEngine> engine;
    QQuickWindow *window = nullptr;
    QQuickItem *menu = nullptr;
    static QQuickItem *visual(QQuickItem *item, const QString &name) {
        if (item->objectName() == name) return item;
        for (auto *child : item->childItems()) if (auto *found = visual(child, name)) return found;
        return nullptr;
    }
    void click(const QString &name) {
        auto *item = visual(window->contentItem(), name);
        QVERIFY2(item, qPrintable(name));
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
            item->mapToScene(QPointF(item->width()/2, item->height()/2)).toPoint());
    }
    void write(const QString &path, const QByteArray &bytes) {
        QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(bytes), qint64(bytes.size()));
    }
    void load(int width = 1280, int height = 720, double scale = 1, bool dark = true) {
        warnings.clear();
        applications = std::make_unique<AppModel>();
        controller = std::make_unique<ShellController>(true, true);
        controller->setDarkTheme(dark);
        desktop = std::make_unique<DesktopModel>(home.filePath("Desktop"));
        notifications = std::make_unique<NotificationService>();
        engine = std::make_unique<QQmlApplicationEngine>();
        connect(engine.get(), &QQmlApplicationEngine::warnings, this, [this](const QList<QQmlError> &errors) {
            for (const auto &error : errors) warnings.append(error.toString());
        });
        engine->addImageProvider("theme", new ThemeIconProvider);
        auto *context = engine->rootContext();
        context->setContextProperty("Applications", applications.get());
        context->setContextProperty("Controller", controller.get());
        context->setContextProperty("DesktopItems", desktop.get());
        context->setContextProperty("Notifications", notifications.get());
        context->setContextProperty("CaptureWidth", width);
        context->setContextProperty("CaptureHeight", height);
        context->setContextProperty("CaptureScale", scale);
        const QString source = qEnvironmentVariable("FIDAN_LAUNCHER_QML", QString(FIDAN_SOURCE_DIR) + "/src/shell/qml/components/AppLauncher.qml");
        context->setContextProperty("LauncherSource", QUrl::fromLocalFile(source));
        context->setContextProperty("Wallpaper", QUrl::fromLocalFile(QString(FIDAN_SOURCE_DIR) + "/artwork/wallpapers/fidan-mountain-dawn-4k.png"));
        engine->loadData(R"QML(import QtQuick
Window {
    id: captureWindow
    width: CaptureWidth; height: CaptureHeight; visible: true; color: "#14221e"
    Image { anchors.fill: parent; source: Wallpaper; fillMode: Image.PreserveAspectCrop }
    Rectangle { anchors.fill: parent; color: "#2805100b" }
    Loader {
        objectName: "launcherLoader"; anchors.centerIn: parent; source: LauncherSource
        onLoaded: item.scale = Qt.binding(function() { return Math.min(CaptureScale, (captureWindow.width - 40) / item.implicitWidth, (captureWindow.height - 100) / item.implicitHeight) })
    }
})QML");
        QVERIFY2(!engine->rootObjects().isEmpty(), qPrintable(warnings.join('\n')));
        window = qobject_cast<QQuickWindow *>(engine->rootObjects().first()); QVERIFY(window);
        auto *loader = window->findChild<QObject *>("launcherLoader"); QVERIFY(loader);
        menu = qvariant_cast<QQuickItem *>(loader->property("item"));
        QVERIFY2(menu, qPrintable(warnings.join('\n')));
        controller->setLauncherVisible(true);
        window->requestActivate(); QTest::qWait(200);
    }
#endif
private slots:
    void initTestCase() {
        QVERIFY(home.isValid());
        QTest::failOnWarning(QRegularExpression(".*(Binding loop|ReferenceError|TypeError).*"));
        qputenv("HOME", home.path().toUtf8());
        qputenv("XDG_CONFIG_HOME", home.filePath("config").toUtf8());
        qputenv("XDG_DATA_HOME", home.filePath("data").toUtf8());
        QQuickStyle::setStyle("Basic");
        qmlRegisterSingletonType(QUrl::fromLocalFile(QString(FIDAN_SOURCE_DIR) + "/src/shell/qml/Theme.qml"), "Fidan.Shell", 1, 0, "Theme");
        const QString apps = home.filePath("data/applications/");
        for (const auto &id : {"org.fidan.Files", "org.fidan.Settings", "org.fidan.Store", "org.fidan.TaskManager", "org.fidan.Photos", "org.fidan.Calculator"}) {
            QFile source(QString(FIDAN_SOURCE_DIR) + "/data/applications/" + id + ".desktop");
            QVERIFY(source.open(QIODevice::ReadOnly)); write(apps + id + ".desktop", source.readAll());
        }
        for (const auto &entry : QList<QStringList>{{"qterminal", "Terminal", "utilities-terminal", "System"}, {"chromium", "Web Browser", "web-browser", "Network"}, {"vlc", "Media Player", "vlc", "AudioVideo"}, {"code", "Code Studio", "visual-studio-code", "Development"}, {"launcher-test-editor", "Launcher Fixture Editor", "accessories-text-editor", "Development"}}) {
            write(apps + entry[0] + ".desktop", QString("[Desktop Entry]\nType=Application\nName=%1\nIcon=%2\nExec=/bin/true\nCategories=%3;\n").arg(entry[1], entry[2], entry[3]).toUtf8());
        }
        QByteArray xbel;
        QXmlStreamWriter xml(&xbel); xml.writeStartDocument(); xml.writeStartElement("xbel");
        for (const auto &name : {"Project Ideas.txt", "Raspberry Pi Setup Guide.pdf", "wallpaper.jpg", "GPIO Blinker.py", "PiOS Development.txt"}) {
            const QString path = home.filePath("Documents/" + QString::fromLatin1(name)); write(path, "fixture");
            xml.writeStartElement("bookmark"); xml.writeAttribute("href", QUrl::fromLocalFile(path).toString());
            xml.writeAttribute("modified", QDateTime::currentDateTimeUtc().addSecs(-60).toString(Qt::ISODate)); xml.writeEndElement();
        }
        xml.writeStartElement("bookmark"); xml.writeAttribute("href", "https://example.invalid/remote"); xml.writeEndElement();
        xml.writeEndElement(); xml.writeEndDocument(); write(home.filePath("data/recently-used.xbel"), xbel);
    }
    void cleanup() {
        const auto actual = warnings;
        window = nullptr; menu = nullptr; engine.reset(); notifications.reset(); desktop.reset(); controller.reset(); applications.reset();
        QVERIFY2(actual.isEmpty(), qPrintable(actual.join('\n')));
    }
    void categoriesPinsAndRecentFiles() {
        AppModel model;
        model.setFilter("Launcher Fixture"); model.setCategory("Development"); QCOMPARE(model.rowCount(), 1);
        model.setCategory("Network"); QCOMPARE(model.rowCount(), 0);
        QVERIFY(!model.togglePinned("missing.desktop"));
        const QString id = "launcher-test-editor.desktop";
        model.setCategory("Development");
        QVERIFY(model.togglePinned(id)); model.setCategory("Pinned"); QCOMPARE(model.rowCount(), 1);
        AppModel restored; restored.setCategory("Pinned"); restored.setFilter("Launcher Fixture"); QCOMPARE(restored.rowCount(), 1);
        QVERIFY(restored.togglePinned(id)); restored.setCategory("Pinned"); QCOMPARE(restored.rowCount(), 0);
        QCOMPARE(model.recentFiles().size(), 5);
        LauncherUrlProbe probe; QDesktopServices::setUrlHandler("file", &probe, "capture");
        QVERIFY(model.openRecentFile(0)); QCOMPARE(QFileInfo(probe.opened.toLocalFile()).fileName(), "Project Ideas.txt");
        QDesktopServices::unsetUrlHandler("file"); QVERIFY(!model.openRecentFile(-1)); QVERIFY(!model.openRecentFile(99));
        QSettings settings(home.filePath("config/fidan/shell.conf"), QSettings::IniFormat); settings.remove("launcher/pinnedApps");
    }
    void rejectsPrivateMissingAndMalformedHistory() {
        const QString path = home.filePath("data/recently-used.xbel");
        QFile history(path); QVERIFY(history.open(QIODevice::ReadOnly)); const auto original = history.readAll(); history.close();
        const auto restore = qScopeGuard([&] { write(path, original); });
        const QString visible = home.filePath("Documents/Visible.txt"); write(visible, "visible");
        const QString privatePath = home.filePath("Documents/Private.txt"); write(privatePath, "private");
        const QString launcher = home.filePath("Documents/untrusted.desktop"); write(launcher, "[Desktop Entry]\nExec=/bin/true\n");
        QByteArray bytes; QXmlStreamWriter xml(&bytes); xml.writeStartElement("xbel");
        for (const auto &file : {visible, visible, privatePath, launcher, home.filePath("missing.txt")}) {
            xml.writeStartElement("bookmark"); xml.writeAttribute("href", QUrl::fromLocalFile(file).toString());
            if (file == privatePath) xml.writeEmptyElement("private");
            xml.writeEndElement();
        }
        xml.writeEndElement(); write(path, bytes);
        AppModel model; QCOMPARE(model.recentFiles().size(), 1); QCOMPARE(model.recentFiles().first().toMap().value("name").toString(), "Visible.txt");
        QVERIFY(QFile::remove(visible)); QVERIFY(!model.openRecentFile(0));
        write(path, "<xbel><bookmark href='file:///tmp/missing'"); model.refresh(); QVERIFY(model.recentFiles().isEmpty());
    }
    void keyboardAndNavigation() {
        load(); QVERIFY(window && menu);
        auto *search = visual(window->contentItem(), "launcher-search"); QVERIFY(search); QVERIFY(search->hasActiveFocus());
        click("launcher-category-Development"); QCOMPARE(applications->category(), "Development");
        search->setProperty("text", "Launcher Fixture"); QCOMPARE(applications->rowCount(), 1);
        search->forceActiveFocus(); QTest::keyClick(window, Qt::Key_Down);
        auto *grid = visual(window->contentItem(), "launcher-app-grid"); QVERIFY(grid); QVERIFY(grid->hasActiveFocus());
        QTest::keyClick(window, Qt::Key_Return); QVERIFY(!controller->launcherVisible());
        controller->setLauncherVisible(true); QCOMPARE(menu->property("page").toString(), "home"); QCOMPARE(search->property("text").toString(), "");
        click("launcher-recent"); QCOMPARE(menu->property("page").toString(), "recent");
        search->setProperty("text", "wallpaper");
        QCOMPARE(menu->property("matchingFiles").value<QJSValue>().toVariant().toList().size(), 1);
        QTest::keyClick(window, Qt::Key_Escape); QCOMPARE(search->property("text").toString(), ""); QVERIFY(controller->launcherVisible());
        QTest::keyClick(window, Qt::Key_Escape); QVERIFY(!controller->launcherVisible());
    }
    void renders_data() {
        QTest::addColumn<int>("width"); QTest::addColumn<int>("height"); QTest::addColumn<double>("scale"); QTest::addColumn<bool>("dark");
        for (bool dark : {true, false}) {
            const QString prefix = dark ? "dark-" : "light-";
            QTest::newRow(qPrintable(prefix + "1280x720")) << 1280 << 720 << 1.0 << dark;
            QTest::newRow(qPrintable(prefix + "1920x1080")) << 1920 << 1080 << 1.0 << dark;
            QTest::newRow(qPrintable(prefix + "3840x2160-2x")) << 3840 << 2160 << 2.0 << dark;
        }
    }
    void renders() {
        QFETCH(int, width); QFETCH(int, height); QFETCH(double, scale); QFETCH(bool, dark);
        load(width, height, scale, dark); QVERIFY(window && menu);
        QTest::qWait(300);
        const double expectedScale = std::min({scale, (width - 40.0) / menu->implicitWidth(), (height - 100.0) / menu->implicitHeight()});
        QVERIFY(menu->scale() > 0); QVERIFY(qAbs(menu->scale() - expectedScale) < 0.001);
        const QRectF bounds = menu->mapRectToScene(menu->boundingRect());
        QVERIFY(bounds.left() >= 0 && bounds.top() >= 0 && bounds.right() <= width && bounds.bottom() <= height);
        const auto screenshot = window->grabWindow(); QVERIFY(!screenshot.isNull());
        const QString output = qEnvironmentVariable("FIDAN_LAUNCHER_CAPTURE_DIR");
        if (!output.isEmpty()) { QVERIFY(QDir().mkpath(output)); QVERIFY(screenshot.save(output + "/" + QTest::currentDataTag() + ".png")); }
    }
    void turkishLabels() {
        QTranslator translator;
        QVERIFY(translator.load(QCoreApplication::applicationDirPath() + "/../src/shell/fidan-shell_tr.qm"));
        QCoreApplication::installTranslator(&translator);
        const auto restore = qScopeGuard([&] { QCoreApplication::removeTranslator(&translator); });
        load(1920, 1080); QVERIFY(window && menu);
        auto *all = visual(window->contentItem(), "launcher-category-All"); QVERIFY(all);
        QCOMPARE(all->property("text").toString(), QString::fromUtf8("Tüm uygulamalar"));
        const QString output = qEnvironmentVariable("FIDAN_LAUNCHER_CAPTURE_DIR");
        if (!output.isEmpty()) { QTest::qWait(200); QVERIFY(window->grabWindow().save(output + "/tr-dark-1920x1080.png")); }
    }
};
QTEST_MAIN(LauncherUiTest)
#include "tst_launcher_ui.moc"
