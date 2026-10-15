#include "../src/common/AppStyle.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "FilesModel.h"
#include "ThemeIconProvider.h"

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QJSValue>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>
#include <memory>

class FilesUiTest final : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QQuickStyle::setStyle(QStringLiteral("Basic"));
    }
    void init();
    void cleanup();
    void createsFileWithKeyboard();
    void keepsInvalidCreationDialogOpen();
    void focusesSearchAndPathWithKeyboard();
    void resetsSearchAndInvalidPath();
    void navigatesWithKeyboard();
    void renamesWithKeyboard();
    void refreshesExternalChanges();
    void maximizesRestoresAndCloses();
    void copiesMultipleFilesWithKeyboard();
    void opensPropertiesWithKeyboard();
    void dropsFileIntoFolder();

private:
    std::unique_ptr<QTemporaryDir> directory;
    std::unique_ptr<FilesModel> files;
    std::unique_ptr<QQmlApplicationEngine> engine;
    QQuickWindow *window = nullptr;
};


void FilesUiTest::dropsFileIntoFolder()
{
    QTemporaryDir source;
    QFile original(source.filePath(QStringLiteral("Dropped note.txt")));
    QVERIFY(original.open(QIODevice::WriteOnly)); original.write("drop content"); original.close();
    QVERIFY(files->createFolder(QStringLiteral("Drop target")));
    auto *grid = window->findChild<QQuickItem *>(QStringLiteral("fileGrid"));
    QVERIFY(grid); grid->setProperty("currentIndex", 0);
    QTest::qWait(30);
    auto *item = grid->property("currentItem").value<QQuickItem *>();
    QVERIFY(item);
    const QPoint point = item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
    QMimeData mime; mime.setUrls({QUrl::fromLocalFile(original.fileName())});
    QDragEnterEvent enter(point, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &enter);
    QDropEvent drop(point, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &drop);
    QTRY_VERIFY_WITH_TIMEOUT(!files->busy(), 5000);
    const QString copied = directory->filePath(QStringLiteral("Drop target/Dropped note.txt"));
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(copied), 5000);
    QFile result(copied); QVERIFY(result.open(QIODevice::ReadOnly)); QCOMPARE(result.readAll(), QByteArray("drop content"));
    QVERIFY(QFileInfo::exists(original.fileName()));
}

void FilesUiTest::copiesMultipleFilesWithKeyboard()
{
    QVERIFY(files->createFile(QStringLiteral("first.txt")));
    QVERIFY(files->createFile(QStringLiteral("second.txt")));
    QTemporaryDir destination;
    auto *grid = window->findChild<QQuickItem *>(QStringLiteral("fileGrid"));
    auto *path = window->findChild<QQuickItem *>(QStringLiteral("folderPath"));
    QVERIFY(grid && path);
    grid->forceActiveFocus();
    QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
    QCOMPARE(window->property("selectedRows").value<QJSValue>().toVariant().toList().size(), 2);
    QTest::keyClick(window, Qt::Key_C, Qt::ControlModifier);
    QVERIFY(files->canPaste());
    QTest::keyClick(window, Qt::Key_L, Qt::ControlModifier);
    QTRY_VERIFY(path->hasActiveFocus());
    path->setProperty("text", destination.path());
    QTest::keyClick(window, Qt::Key_Return);
    QTRY_VERIFY(grid->hasActiveFocus());
    QTest::keyClick(window, Qt::Key_V, Qt::ControlModifier);
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(destination.filePath(QStringLiteral("second.txt"))), 5000);
    QTRY_VERIFY(!files->busy());
    QVERIFY(QFileInfo::exists(destination.filePath(QStringLiteral("first.txt"))));
    QVERIFY(QFileInfo::exists(directory->filePath(QStringLiteral("first.txt"))));
}

void FilesUiTest::opensPropertiesWithKeyboard()
{
    QVERIFY(files->createFile(QStringLiteral("notes.txt")));
    auto *grid = window->findChild<QQuickItem *>(QStringLiteral("fileGrid"));
    QVERIFY(grid); grid->setProperty("currentIndex", 0); grid->forceActiveFocus();
    QTest::keyClick(window, Qt::Key_Return, Qt::AltModifier);
    auto *dialog = window->findChild<QObject *>(QStringLiteral("propertiesDialog"));
    QVERIFY(dialog); QTRY_VERIFY(dialog->property("visible").toBool());
    QTest::keyClick(window, Qt::Key_Escape);
    QTRY_VERIFY(!dialog->property("visible").toBool());
}

void FilesUiTest::init()
{
    directory = std::make_unique<QTemporaryDir>();
    QVERIFY(directory->isValid());
    files = std::make_unique<FilesModel>(directory->path());
    engine = std::make_unique<QQmlApplicationEngine>();
    auto *style = new AppStyle(engine.get());
    engine->rootContext()->setContextProperty("UiStyle", style);
    engine->addImageProvider(QStringLiteral("theme"), new ThemeIconProvider);
    engine->rootContext()->setContextProperty(QStringLiteral("filesBackend"), files.get());
    engine->load(QUrl::fromLocalFile(QStringLiteral(FIDAN_SOURCE_DIR "/src/files/qml/Files.qml")));
    QVERIFY2(!engine->rootObjects().isEmpty(), "Files QML could not be loaded");
    window = qobject_cast<QQuickWindow *>(engine->rootObjects().first());
    QVERIFY(window);
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(QTest::qWaitForWindowActive(window));
}

void FilesUiTest::cleanup()
{
    if (window) window->close();
    window = nullptr;
    engine.reset();
    files.reset();
    directory.reset();
}

void FilesUiTest::createsFileWithKeyboard()
{
    auto *dialog = window->findChild<QObject *>(QStringLiteral("createDialog"));
    auto *name = window->findChild<QQuickItem *>(QStringLiteral("newEntryName"));
    QVERIFY(dialog && name);
    QTest::keyClick(window, Qt::Key_N, Qt::ControlModifier);
    QTRY_VERIFY(dialog->property("visible").toBool());
    QTRY_VERIFY(name->hasActiveFocus());
    QVERIFY(name->setProperty("text", QStringLiteral("Keyboard notes.txt")));
    QTest::keyClick(window, Qt::Key_Return);
    QTRY_VERIFY(QFileInfo::exists(directory->filePath(QStringLiteral("Keyboard notes.txt"))));
    QTRY_VERIFY(!dialog->property("visible").toBool());
    QCOMPARE(files->rowCount(), 1);
}

void FilesUiTest::keepsInvalidCreationDialogOpen()
{
    auto *dialog = window->findChild<QObject *>(QStringLiteral("createDialog"));
    auto *name = window->findChild<QQuickItem *>(QStringLiteral("newEntryName"));
    QVERIFY(dialog && name);
    QTest::keyClick(window, Qt::Key_N, Qt::ControlModifier | Qt::ShiftModifier);
    QTRY_VERIFY(dialog->property("visible").toBool());
    QTRY_VERIFY(name->hasActiveFocus());
    name->setProperty("text", QStringLiteral("../outside"));
    QTest::keyClick(window, Qt::Key_Return);
    QVERIFY(!files->errorText().isEmpty());
    QVERIFY(dialog->property("visible").toBool());
    QCOMPARE(files->rowCount(), 0);
    QTest::keyClick(window, Qt::Key_Escape);
    QTRY_VERIFY(!dialog->property("visible").toBool());
}

void FilesUiTest::focusesSearchAndPathWithKeyboard()
{
    auto *search = window->findChild<QQuickItem *>(QStringLiteral("folderSearch"));
    auto *path = window->findChild<QQuickItem *>(QStringLiteral("folderPath"));
    QVERIFY(search && path);
    QTest::keyClick(window, Qt::Key_F, Qt::ControlModifier);
    QTRY_VERIFY(search->hasActiveFocus());
    QTest::keyClick(window, Qt::Key_L, Qt::ControlModifier);
    QTRY_VERIFY(path->hasActiveFocus());
    QCOMPARE(path->property("selectedText").toString(), files->path());
}

void FilesUiTest::resetsSearchAndInvalidPath()
{
    auto *search = window->findChild<QQuickItem *>(QStringLiteral("folderSearch"));
    auto *path = window->findChild<QQuickItem *>(QStringLiteral("folderPath"));
    QVERIFY(search && path);
    search->setProperty("text", QStringLiteral("missing"));
    QCOMPARE(files->filter(), QStringLiteral("missing"));
    QVERIFY(QDir(directory->path()).mkdir(QStringLiteral("Projects")));
    files->setPath(directory->filePath(QStringLiteral("Projects")));
    QCOMPARE(search->property("text").toString(), QString());
    path->forceActiveFocus();
    path->setProperty("text", directory->filePath(QStringLiteral("nonexistent")));
    QTest::keyClick(window, Qt::Key_Return);
    QCOMPARE(path->property("text").toString(), files->path());
    QVERIFY(!files->errorText().isEmpty());
}

void FilesUiTest::navigatesWithKeyboard()
{
    QVERIFY(files->createFolder(QStringLiteral("Projects")));
    auto *grid = window->findChild<QQuickItem *>(QStringLiteral("fileGrid"));
    QVERIFY(grid);
    grid->setProperty("currentIndex", 0);
    grid->forceActiveFocus();
    QTest::keyClick(window, Qt::Key_Return);
    QTRY_COMPARE(files->path(), directory->filePath(QStringLiteral("Projects")));
    QTest::keyClick(window, Qt::Key_Left, Qt::AltModifier);
    QTRY_COMPARE(files->path(), directory->path());
    QTest::keyClick(window, Qt::Key_Right, Qt::AltModifier);
    QTRY_COMPARE(files->path(), directory->filePath(QStringLiteral("Projects")));
    QTest::keyClick(window, Qt::Key_Up, Qt::AltModifier);
    QTRY_COMPARE(files->path(), directory->path());
}

void FilesUiTest::renamesWithKeyboard()
{
    QVERIFY(files->createFile(QStringLiteral("before.txt")));
    auto *grid = window->findChild<QQuickItem *>(QStringLiteral("fileGrid"));
    auto *name = window->findChild<QQuickItem *>(QStringLiteral("renameEntryName"));
    QVERIFY(grid && name);
    grid->setProperty("currentIndex", 0);
    grid->forceActiveFocus();
    QTest::keyClick(window, Qt::Key_F2);
    QTRY_VERIFY(name->hasActiveFocus());
    name->setProperty("text", QStringLiteral("after.txt"));
    QTest::keyClick(window, Qt::Key_Return);
    QTRY_VERIFY(QFileInfo::exists(directory->filePath(QStringLiteral("after.txt"))));
    QVERIFY(!QFileInfo::exists(directory->filePath(QStringLiteral("before.txt"))));
}

void FilesUiTest::refreshesExternalChanges()
{
    QCOMPARE(files->rowCount(), 0);
    QFile external(directory->filePath(QStringLiteral("external.txt")));
    QVERIFY(external.open(QIODevice::WriteOnly));
    external.close();
    QTest::keyClick(window, Qt::Key_F5);
    QTRY_COMPARE(files->rowCount(), 1);
}

void FilesUiTest::maximizesRestoresAndCloses()
{
    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(250, 20));
    QTRY_COMPARE(window->visibility(), QWindow::Maximized);
    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(250, 20));
    QTRY_COMPARE(window->visibility(), QWindow::Windowed);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(window->width() - 23, 20));
    QTRY_VERIFY(!window->isVisible());
}

QTEST_MAIN(FilesUiTest)
#include "tst_files_ui.moc"
