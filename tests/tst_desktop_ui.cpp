// SPDX-License-Identifier: GPL-3.0-or-later
#include "DesktopModel.h"
#include "FilesModel.h"
#include "ThemeIconProvider.h"
#include <QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QJSValue>
#include <QTemporaryDir>
#include <QFile>
#include <QDragEnterEvent>
#include <QDrag>
#include <QDropEvent>
#include <QMimeData>
#include <QTimer>

class DesktopController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool previewMode READ previewMode CONSTANT)
    Q_PROPERTY(double interfaceScale MEMBER scale NOTIFY changed)
    Q_PROPERTY(bool desktopIconsVisible READ iconsVisible CONSTANT)
    Q_PROPERTY(bool darkTheme READ darkTheme CONSTANT)
    Q_PROPERTY(QString accentColor READ accentColor CONSTANT)
    Q_PROPERTY(bool reducedMotion READ reducedMotion CONSTANT)
public:
    double scale=1;
    bool previewMode() const { return false; }
    bool iconsVisible() const { return true; }
    bool darkTheme() const { return true; }
    QString accentColor() const { return "#63de8a"; }
    bool reducedMotion() const { return true; }
signals:
    void changed();
};

class DesktopUiTest final : public QObject {
    Q_OBJECT
    QTemporaryDir directory;
    std::unique_ptr<DesktopModel> desktop;
    std::unique_ptr<FilesModel> files;
    std::unique_ptr<QQmlApplicationEngine> engine;
    DesktopController controller;
    QQuickWindow *window=nullptr;
    QQuickItem *icons=nullptr;
    QStringList warnings;
    static QQuickItem *visual(QQuickItem *item, const QString &name) {
        if(item->objectName()==name) return item;
        for(auto *child:item->childItems()) if(auto *found=visual(child,name)) return found;
        return nullptr;
    }
    void write(const QString &name,const QByteArray &content="fixture") {
        QFile file(directory.filePath(name)); QVERIFY(file.open(QIODevice::WriteOnly)); file.write(content); file.close();
    }
    QPoint point(const QString &path) {
        const int row=desktop->paths().indexOf(path);
        auto *item=visual(window->contentItem(),"desktop-icon-"+QString::number(row));
        return item ? item->mapToScene(QPointF(item->width()/2,item->height()/2)).toPoint() : QPoint(-1,-1);
    }
    static QStringList strings(const QVariant &value) {
        return value.metaType()==QMetaType::fromType<QJSValue>() ? value.value<QJSValue>().toVariant().toStringList() : value.toStringList();
    }
    QStringList selection() const { return strings(icons->property("selectedPaths")); }
    void drop(const QList<QUrl> &urls,const QPoint &position,Qt::DropAction action) {
        QMimeData mime; mime.setUrls(urls);
        QDragEnterEvent enter(position,action,&mime,Qt::LeftButton,Qt::NoModifier); QCoreApplication::sendEvent(window,&enter);
        QDropEvent event(position,action,&mime,Qt::LeftButton,Qt::NoModifier); QCoreApplication::sendEvent(window,&event);
    }
private slots:
    void initTestCase() {
        QVERIFY(directory.isValid());
        qmlRegisterSingletonType(QUrl::fromLocalFile(QStringLiteral(FIDAN_SOURCE_DIR "/src/shell/qml/Theme.qml")),"Fidan.Shell",1,0,"Theme");
    }
    void init() {
        warnings.clear(); controller.scale=1;
        QVERIFY(QDir(directory.path()).mkdir("Folder"));
        write("a note.txt"); write(QString::fromUtf8("b #Çağrı.txt")); write("c.desktop","[Desktop Entry]\nType=Application\nName=App\nExec=/bin/true\n");
        desktop=std::make_unique<DesktopModel>(directory.path()); files=std::make_unique<FilesModel>(directory.path());
        engine=std::make_unique<QQmlApplicationEngine>();
        connect(engine.get(),&QQmlApplicationEngine::warnings,this,[&](const QList<QQmlError> &errors){ for(const auto &error:errors) warnings.append(error.toString()); });
        engine->addImageProvider("theme",new ThemeIconProvider);
        auto *context=engine->rootContext(); context->setContextProperty("Controller",&controller); context->setContextProperty("DesktopItems",desktop.get()); context->setContextProperty("DesktopFiles",files.get());
        engine->loadData("import QtQuick\nimport \".\"\nWindow { width:800; height:500; visible:true; DesktopIcons { objectName: 'icons'; anchors.fill:parent } }",QUrl::fromLocalFile(QStringLiteral(FIDAN_SOURCE_DIR "/src/shell/qml/components/test-desktop.qml")));
        QVERIFY2(!engine->rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
        window=qobject_cast<QQuickWindow *>(engine->rootObjects().first()); QVERIFY(window);
        window->requestActivate(); QTest::qWait(80); icons=visual(window->contentItem(),"icons"); QVERIFY(icons);
    }
    void cleanup() {
        const auto actualWarnings=warnings;
        engine.reset(); files.reset(); desktop.reset();
        for(const auto &file:QDir(directory.path()).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot)) {
            if(file.isDir()) QVERIFY(QDir(file.absoluteFilePath()).removeRecursively()); else QVERIFY(QFile::remove(file.absoluteFilePath()));
        }
        QVERIFY2(actualWarnings.isEmpty(),qPrintable(actualWarnings.join('\n')));
    }
    void boxSelectsWithoutMovingDesktop() {
        auto *grid=visual(window->contentItem(),"desktop-grid"); QVERIFY(grid); QVERIFY(!grid->property("interactive").toBool());
        QTest::mousePress(window,Qt::LeftButton,Qt::NoModifier,QPoint(270,450));
        QTest::mouseMove(window,QPoint(2,2),40); QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,QPoint(2,2));
        QCOMPARE(selection().size(),4); QCOMPARE(grid->property("contentX").toDouble(),0.0); QCOMPARE(grid->property("contentY").toDouble(),0.0);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,QPoint(650,450)); QCOMPARE(selection().size(),0);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,point(directory.filePath("a note.txt"))); QCOMPARE(selection().size(),1);
        QTest::keyClick(window,Qt::Key_A,Qt::ControlModifier); QCOMPARE(selection().size(),4);
        QTest::keyClick(window,Qt::Key_Escape); QCOMPARE(selection().size(),0);
    }
    void preservesSelectionByPathThroughSortAndRefresh() {
        const QString a=directory.filePath("a note.txt"), b=directory.filePath(QString::fromUtf8("b #Çağrı.txt"));
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,point(a));
        QTest::mouseClick(window,Qt::LeftButton,Qt::ControlModifier,point(b)); QCOMPARE(selection().size(),2);
        desktop->setSortMode("Type"); QTest::qWait(50); QVERIFY(selection().contains(a)); QVERIFY(selection().contains(b));
        write("new.txt"); desktop->refresh(); QTest::qWait(50); QCOMPARE(selection().size(),2);
        QTest::mouseClick(window,Qt::LeftButton,Qt::ControlModifier,point(a)); QCOMPARE(selection(),QStringList{b});
    }
    void movesMultipleItemsIntoFolder() {
        const QString a=directory.filePath("a note.txt"), b=directory.filePath(QString::fromUtf8("b #Çağrı.txt"));
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,point(a));
        QTest::mouseClick(window,Qt::LeftButton,Qt::ControlModifier,point(b));
        QList<QUrl> urls; for(const auto &url:desktop->dragUrls(selection())) urls.append(QUrl(url)); QCOMPARE(urls.size(),2);
        QCOMPARE(urls.last().toLocalFile(),b);
        drop(urls,point(directory.filePath("Folder")),Qt::MoveAction);
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(directory.filePath("Folder/a note.txt")),3000);
        QTRY_VERIFY(!files->busy()); QVERIFY(!QFileInfo::exists(a)); QVERIFY(!QFileInfo::exists(b));
        QVERIFY(QFileInfo::exists(directory.filePath(QString::fromUtf8("Folder/b #Çağrı.txt"))));
        desktop->refresh(); QTRY_VERIFY(selection().isEmpty());
    }
    void rejectsSelfRecursiveConflictingAndRemoteDrops() {
        const QString folder=directory.filePath("Folder"), a=directory.filePath("a note.txt");
        QVERIFY(!desktop->canDrop({QUrl::fromLocalFile(folder)},folder));
        QVERIFY(QDir(folder).mkdir("Nested")); QVERIFY(!desktop->canDrop({QUrl::fromLocalFile(folder)},folder+"/Nested"));
        write("Folder/a note.txt","original"); QVERIFY(!desktop->canDrop({QUrl::fromLocalFile(a)},folder));
        QVERIFY(!desktop->canDrop({QUrl("https://example.invalid/item")},folder));
        drop({QUrl::fromLocalFile(a)},point(folder),Qt::MoveAction); QTest::qWait(80);
        QVERIFY(QFileInfo::exists(a)); QFile existing(folder+"/a note.txt"); QVERIFY(existing.open(QIODevice::ReadOnly)); QCOMPARE(existing.readAll(),QByteArray("original"));
    }
    void movedApplicationShortcutRemainsLaunchable() {
        const QString source=directory.filePath("c.desktop");
        const QString marker=directory.filePath("launched");
        write("c.desktop",("[Desktop Entry]\nType=Application\nName=App\nExec=/usr/bin/touch "+marker+"\n").toUtf8());
        QVERIFY(QFile::setPermissions(source,QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner));
        drop({QUrl::fromLocalFile(source)},point(directory.filePath("Folder")),Qt::MoveAction);
        QTRY_VERIFY_WITH_TIMEOUT(!QFileInfo::exists(source),3000); QTRY_VERIFY(!files->busy());
        files->setPath(directory.filePath("Folder")); QCOMPARE(files->rowCount(),1);
        files->activate(0); QTRY_VERIFY(QFileInfo::exists(marker));
    }
    void dragPayloadSurvivesSourceRemoval() {
        const QString source=directory.filePath("a note.txt");
        const QPoint start=point(source), end=start+QPoint(45,5);
        bool completed=false;
        QTimer::singleShot(100,this,[&,end] {
            files->dropUrls({QUrl::fromLocalFile(source)},directory.filePath("Folder"),true);
            QTimer::singleShot(100,this,[&,end] {
                QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,end);
                completed=true;
            });
        });
        QTest::mousePress(window,Qt::LeftButton,Qt::NoModifier,start);
        QTest::mouseMove(window,start+QPoint(20,0),20);
        QTest::mouseMove(window,end,20);
        QTest::qWait(50);
        const QStringList payload{QUrl::fromLocalFile(source).toString(QUrl::FullyEncoded)};
        QTRY_VERIFY(completed);
        QTRY_VERIFY(!files->busy());
        QVERIFY(!QFileInfo::exists(source)); QVERIFY(QFileInfo::exists(directory.filePath("Folder/a note.txt")));
        QVERIFY(window->isVisible()); QVERIFY(icons);
        QCOMPARE(strings(icons->property("draggedUrls")),payload);
        QVERIFY(!icons->property("dragInProgress").toBool());
    }
    void copiesExternalItemOntoDesktopAndUsesScaledSelection() {
        QTemporaryDir source; QFile file(source.filePath("external.txt")); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("external"); file.close();
        drop({QUrl::fromLocalFile(file.fileName())},QPoint(600,400),Qt::CopyAction);
        QTRY_VERIFY(QFileInfo::exists(directory.filePath("external.txt"))); QTRY_VERIFY(!files->busy()); QVERIFY(QFileInfo::exists(file.fileName()));
        desktop->refresh(); controller.scale=1.5; emit controller.changed(); QTest::qWait(80);
        QTest::mousePress(window,Qt::LeftButton,Qt::NoModifier,QPoint(350,490)); QTest::mouseMove(window,QPoint(1,1),40); QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,QPoint(1,1));
        QCOMPARE(selection().size(),5);
    }
};
QTEST_MAIN(DesktopUiTest)
#include "tst_desktop_ui.moc"
