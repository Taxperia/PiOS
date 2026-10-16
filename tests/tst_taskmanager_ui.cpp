// SPDX-License-Identifier: GPL-3.0-or-later
#include "ProcessModel.h"
#include "AppStyle.h"
#include "ThemeIconProvider.h"
#include <QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QTemporaryDir>
#include <QSettings>
#include <QProcess>

class TaskManagerTest final : public QObject {
    Q_OBJECT
    QTemporaryDir config;
    static QQuickItem *visual(QQuickItem *item, const QString &name) {
        if (item->objectName() == name) return item;
        for (auto *child : item->childItems()) if (auto *found = visual(child, name)) return found;
        return nullptr;
    }
    static void click(QQuickWindow *window, QQuickItem *item) {
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, item->mapToScene(QPointF(item->width()/2, item->height()/2)).toPoint());
    }
private slots:
    void initTestCase() {
        QVERIFY(config.isValid()); qputenv("XDG_CONFIG_HOME", config.path().toUtf8()); QQuickStyle::setStyle("Basic");
    }
    void sortsAndPreservesProcessIdentity() {
        QProcess sleeper; sleeper.start("/bin/sleep", {"30"}); QVERIFY(sleeper.waitForStarted());
        ProcessModel model;
        QVERIFY(model.memoryCapacity() > 0); QVERIFY(model.memoryUsed() <= model.memoryCapacity());
        QVERIFY(model.cpuCount() > 0); QVERIFY(!model.cpuName().isEmpty()); QVERIFY(model.uptime() > 0);
        model.sortBy("pid"); QVERIFY(!model.sortDescending());
        for (int i=1; i<model.rowCount(); ++i) QVERIFY(model.data(model.index(i-1), ProcessModel::PidRole).toInt() < model.data(model.index(i), ProcessModel::PidRole).toInt());
        const int pid = sleeper.processId(); const auto details = model.details(pid);
        QCOMPARE(details.value("name").toString(), "sleep");
        model.sortBy("memory"); QVERIFY(model.sortDescending());
        for (int i=1; i<model.rowCount(); ++i) QVERIFY(model.data(model.index(i-1), ProcessModel::MemoryBytesRole).toULongLong() >= model.data(model.index(i), ProcessModel::MemoryBytesRole).toULongLong());
        model.setFilter("definitely-no-such-process-713"); QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.details(pid).value("command"), details.value("command"));
        model.refresh(); QCOMPARE(model.sortColumn(), "memory"); QCOMPARE(model.rowCount(), 0);
        model.setFilter(details.value("user").toString()); QVERIFY(model.rowCount() > 0);
        QVERIFY(model.terminateProcess(pid)); QVERIFY(sleeper.waitForFinished(3000)); model.refresh(); QVERIFY(model.details(pid).isEmpty());
        QVERIFY(!model.terminateProcess(1));
    }
    void navigatesFiltersPausesAndCancels() {
        ProcessModel model; AppStyle style; QQmlApplicationEngine engine; QStringList warnings;
        connect(&engine,&QQmlApplicationEngine::warnings,this,[&](const QList<QQmlError> &errors) { for (const auto &error : errors) warnings.append(error.toString()); });
        engine.addImageProvider("theme",new ThemeIconProvider);
        engine.rootContext()->setContextProperty("processBackend",&model);
        engine.rootContext()->setContextProperty("UiStyle",&style);
        engine.rootContext()->setContextProperty("InitialTaskPage","processes");
        engine.load(QUrl::fromLocalFile(QStringLiteral(FIDAN_SOURCE_DIR "/src/taskmanager/qml/TaskManager.qml")));
        QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
        auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window);
        window->setWidth(880); window->setHeight(540); window->requestActivate(); QTest::qWait(150);
        auto *search=visual(window->contentItem(),"process-search"); QVERIFY(search);
        search->setProperty("text",QString::number(QCoreApplication::applicationPid())); QTest::qWait(100);
        auto *row=visual(window->contentItem(),"process-row-"+QString::number(QCoreApplication::applicationPid())); QVERIFY(row);
        click(window,row); QCOMPARE(window->property("selectedPid").toInt(),QCoreApplication::applicationPid());
        auto *end=visual(window->contentItem(),"end-task"); QVERIFY(end); QVERIFY(end->isEnabled()); click(window,end); QTest::qWait(80);
        QTest::keyClick(window,Qt::Key_Escape); QTest::qWait(80);
        auto *pause=visual(window->contentItem(),"pause-action"); QVERIFY(pause); click(window,pause); QVERIFY(window->property("paused").toBool());
        search->setProperty("text","");
        auto *memory=visual(window->contentItem(),"sort-memory"); QVERIFY(memory); click(window,memory); QCOMPARE(model.sortColumn(),"memory");
        auto *performance=visual(window->contentItem(),"nav-performance"); QVERIFY(performance); click(window,performance); QCOMPARE(window->property("page").toString(),"performance");
        model.refresh(); QTest::qWait(100);
        QSettings preferences(config.filePath("fidan/shell.conf"),QSettings::IniFormat); preferences.setValue("appearance/darkTheme",false); preferences.sync(); QTRY_VERIFY(!style.dark()); QTest::qWait(100);
        click(window,visual(window->contentItem(),"nav-processes"));
        click(window,pause); QVERIFY(!window->property("paused").toBool());
        QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
    }
};
QTEST_MAIN(TaskManagerTest)
#include "tst_taskmanager_ui.moc"
