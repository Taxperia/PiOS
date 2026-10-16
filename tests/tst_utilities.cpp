// SPDX-License-Identifier: GPL-3.0-or-later
#include "UtilityModels.h"
#include "AppStyle.h"
#include "ThemeIconProvider.h"
#include <QtTest>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QPdfSearchModel>
#include <QPdfLinkModel>
#include <QPainter>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QSettings>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQmlExpression>
#include <QSignalSpy>
#include <QFile>
#include <QDir>
#include <QScopeGuard>
#include <QDesktopServices>

class UrlProbe final : public QObject {
    Q_OBJECT
public:
    QList<QUrl> opened;
public slots:
    void capture(const QUrl &url) { opened.append(url); }
};

class UtilitiesTest final : public QObject {
    Q_OBJECT
    QTemporaryDir home;
    QString pdf;
private slots:
    void initTestCase() {
        QVERIFY(home.isValid());
        qputenv("XDG_CONFIG_HOME",home.path().toUtf8()+"/config");
        qputenv("XDG_DATA_HOME",home.path().toUtf8()+"/data");
        QQuickStyle::setStyle("Basic");
        pdf=home.filePath("PiOS belge #Çağrı.pdf");
        QPdfWriter writer(pdf); writer.setTitle("PiOS document test"); writer.setResolution(72);
        QPainter painter(&writer); painter.drawText(80,100,"PiOS PDF search example"); writer.newPage(); painter.drawText(80,100,"Second page"); painter.end();
    }
    void arithmetic_data() {
        QTest::addColumn<QString>("expression"); QTest::addColumn<QString>("expected");
        QTest::newRow("precedence")<<"2+3*4"<<"14";
        QTest::newRow("parentheses")<<"(2+3)*4"<<"20";
        QTest::newRow("right exponent")<<"2^3^2"<<"512";
        QTest::newRow("unary precedence")<<"-2^2"<<"-4";
        QTest::newRow("sqrt percent")<<"sqrt(81)+50%"<<"9.5";
        QTest::newRow("Turkish symbols")<<"12,5 × 2 − 5 ÷ 2"<<"22.5";
        QTest::newRow("scientific")<<"1e-6*1000000"<<"1";
        QTest::newRow("negative power")<<"2^-2"<<"0.25";
    }
    void arithmetic() { QFETCH(QString,expression); QFETCH(QString,expected); CalculatorModel model; QCOMPARE(model.evaluate(expression),expected); QVERIFY(model.error().isEmpty()); }
    void arithmeticRejectsInvalidAndCode() {
        CalculatorModel model;
        for(const QString &text:{QString("1/0"),QString("sqrt(-1)"),QString("1e999"),QString("2+"),QString("process.exit()"),QString("1; touch /tmp/test"),QString(70,'(')+"1"+QString(70,')'),QString(600,'1')}) { QVERIFY2(model.evaluate(text).isEmpty(),qPrintable(text)); QVERIFY(!model.error().isEmpty()); }
        QCOMPARE(model.evaluate("1+2"),QString("3")); QVERIFY(model.error().isEmpty());
    }
    void memoryAndHistoryPersist() {
        CalculatorModel model; model.clearHistory(); model.setMemory("5"); model.setMemory("2",true);
        QCOMPARE(model.memory(),QString("7"));
        for(int i=0;i<35;++i) model.evaluate(QString::number(i)+"+1");
        QCOMPARE(model.history().size(),30); CalculatorModel restored; QCOMPARE(restored.history().size(),30); QCOMPARE(restored.memory(),QString("7"));
        model.clearHistory(); model.clearMemory(); QCOMPARE(model.memory(),QString("0"));
    }
    void calendarPersistenceAndConcurrentWindows() {
        CalendarModel first,second;
        QCOMPARE(first.month(2028,2).size(),42); QCOMPARE(first.month(2028,2).first().toMap().value("date").toString(),QString("2028-01-31"));
        QVERIFY(first.saveEvent("","2028-02-29","14:30","Toplantı #Çağrı"));
        QVERIFY(second.saveEvent("","2028-02-29","","Tüm gün"));
        CalendarModel restored; QCOMPARE(restored.events("2028-02-29").size(),2);
        const auto event=restored.events("2028-02-29").last().toMap();
        QVERIFY(restored.saveEvent(event.value("id").toString(),"2028-03-01","09:00","Düzenlendi"));
        QCOMPARE(restored.events("2028-02-29").size(),1); QCOMPARE(restored.events("2028-03-01").size(),1);
        QVERIFY(!restored.saveEvent("","2027-02-29","10:00","Geçersiz"));
        QVERIFY(!restored.saveEvent("","2028-03-01","24:01","Geçersiz"));
        QVERIFY(!restored.saveEvent("","2028-03-01","10:00","   "));
        QVERIFY(restored.removeEvent(event.value("id").toString())); QCOMPARE(restored.events("2028-03-01").size(),0);
        const QString path=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/fidan/calendar/events.json";
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("broken calendar"); file.close();
        QVERIFY(!restored.saveEvent("","2028-03-01","10:00","Preserve existing")); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(),QByteArray("broken calendar")); file.close(); QVERIFY(file.remove());
    }
    void stopwatchAndCountdown() {
        ClockModel clock; QSignalSpy finished(&clock,&ClockModel::timerFinished);
        QCOMPARE(clock.cities().size(),6); QVERIFY(!clock.time().isEmpty());
        clock.toggleStopwatch(); QTest::qWait(90); clock.lap(); QVERIFY(clock.elapsed()>=80); QCOMPARE(clock.laps().size(),1);
        clock.toggleStopwatch(); const auto paused=clock.elapsed(); QTest::qWait(80); QCOMPARE(clock.elapsed(),paused);
        clock.toggleStopwatch(); QTest::qWait(70); QVERIFY(clock.elapsed()>=paused+60); clock.resetStopwatch(); QCOMPARE(clock.elapsed(),0); QVERIFY(clock.laps().isEmpty());
        clock.startTimer(0); QVERIFY(!clock.counting()); clock.startTimer(1); QVERIFY(clock.counting());
        QTest::qWait(100); clock.pauseTimer(); QVERIFY(!clock.counting()); QTest::qWait(120); QCOMPARE(finished.count(),0); clock.resumeTimer();
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(),1,1800); QCOMPARE(clock.remaining(),0); QVERIFY(!clock.counting());
        QTest::qWait(100); QCOMPARE(finished.count(),1); clock.cancelTimer();
    }
    void realPdfRenderAndSearch() {
        DocumentFiles files; QVERIFY(files.open(QUrl::fromLocalFile(pdf))); QCOMPARE(files.name(),QString("PiOS belge #Çağrı.pdf"));
        const auto previous=files.source(); QVERIFY(!files.open(QUrl("https://example.com/test.pdf"))); QCOMPARE(files.source(),previous);
        QFile bad(home.filePath("not.pdf")); QVERIFY(bad.open(QIODevice::WriteOnly)); bad.write("not a pdf"); bad.close(); QVERIFY(!files.open(QUrl::fromLocalFile(bad.fileName())));
        QPdfDocument document; QCOMPARE(document.load(pdf),QPdfDocument::Error::None); QCOMPARE(document.pageCount(),2);
        QVERIFY(!document.render(0,QSize(300,400)).isNull());
        QPdfSearchModel search; search.setDocument(&document); search.setSearchString("PiOS"); QTRY_VERIFY(search.rowCount(QModelIndex())>0);
    }
    void pdfLinksRejectLocalFilesAndApplicationProtocols() {
        DocumentFiles files; UrlProbe probe;
        const QStringList schemes{"file","https","http","javascript","data","ftp","ms-vscode","mailto"};
        for(const auto &scheme:schemes) QDesktopServices::setUrlHandler(scheme,&probe,"capture");
        const auto cleanup=qScopeGuard([&]{for(const auto &scheme:schemes) QDesktopServices::unsetUrlHandler(scheme);});
        for(const auto &link:{"file:///tmp/untrusted.desktop","javascript:alert(1)","data:text/html,test","ms-vscode://file/tmp/test","mailto:test@example.invalid","ftp://example.invalid/test","https://user:pass@example.invalid/test","/relative/path","https:"}) {
            QVERIFY2(!files.openLink(QUrl(link)),link);QVERIFY(!files.error().isEmpty());QVERIFY(probe.opened.isEmpty());
        }
        QVERIFY(files.openLink(QUrl("https://example.invalid/documentation")));
        QVERIFY(files.error().isEmpty());QCOMPARE(probe.opened,QList<QUrl>{QUrl("https://example.invalid/documentation")});
    }
    void pdfAnnotationCannotBypassLinkPolicy() {
        const QString path=home.filePath("unsafe-link.pdf");
        const QList<QByteArray> objects={
            "<< /Type /Catalog /Pages 2 0 R >>",
            "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
            "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 300 300] /Resources << >> /Contents 4 0 R /Annots [5 0 R] >>",
            "<< /Length 0 >>\nstream\n\nendstream",
            "<< /Type /Annot /Subtype /Link /Rect [20 240 250 280] /A << /S /URI /URI (file:///tmp/untrusted.desktop) >> >>"};
        QByteArray bytes="%PDF-1.7\n";QList<int> offsets{0};
        for(int i=0;i<objects.size();++i){offsets.append(bytes.size());bytes+=QByteArray::number(i+1)+" 0 obj\n"+objects[i]+"\nendobj\n";}
        const auto xref=bytes.size();bytes+="xref\n0 6\n0000000000 65535 f \n";
        for(int i=1;i<offsets.size();++i)bytes+=QByteArray::number(offsets[i]).rightJustified(10,'0')+" 00000 n \n";
        bytes+="trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n"+QByteArray::number(xref)+"\n%%EOF\n";
        QFile fixture(path);QVERIFY(fixture.open(QIODevice::WriteOnly));fixture.write(bytes);fixture.close();
        QPdfDocument parsed;QCOMPARE(parsed.load(path),QPdfDocument::Error::None);
        QPdfLinkModel links;links.setDocument(&parsed);links.setPage(0);
        QTRY_COMPARE(links.rowCount(QModelIndex()),1);
        QCOMPARE(links.data(links.index(0,0),int(QPdfLinkModel::Role::Url)).toUrl(),QUrl("file:///tmp/untrusted.desktop"));
        DocumentFiles files;AppStyle style;QVERIFY(files.open(QUrl::fromLocalFile(path)));
        UrlProbe probe;QDesktopServices::setUrlHandler("file",&probe,"capture");
        const auto cleanup=qScopeGuard([]{QDesktopServices::unsetUrlHandler("file");});
        QQmlApplicationEngine engine;engine.addImageProvider("theme",new ThemeIconProvider);
        engine.rootContext()->setContextProperty("Documents",&files);engine.rootContext()->setContextProperty("UiStyle",&style);
        engine.loadData("import QtQuick\nimport QtQuick.Controls\nApplicationWindow { width:1000; height:700; visible:true; Loader { anchors.fill:parent; source:'PdfViewer.qml' } }",QUrl::fromLocalFile(QString(FIDAN_SOURCE_DIR)+"/src/apps/qml/test-security.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first());QVERIFY(window);
        QObject *annotation=nullptr;
        const auto visit=[&](auto &&self,QQuickItem *item)->QObject * {
            if(item->property("url").toUrl()==QUrl("file:///tmp/untrusted.desktop") && item->property("link").isValid())return item;
            for(auto *child:item->childItems())if(auto *found=self(self,child))return found;
            return nullptr;
        };
        const auto findLink=[&]{annotation=visit(visit,window->contentItem());return annotation!=nullptr;};
        QTRY_VERIFY_WITH_TIMEOUT(findLink(),5000);
        QVERIFY(QMetaObject::invokeMethod(annotation,"tapped",Q_ARG(QVariant,annotation->property("link"))));
        QVERIFY(probe.opened.isEmpty());QVERIFY(!files.error().isEmpty());
        QCOMPARE(files.source(),QUrl::fromLocalFile(path));window->close();
    }
    void pagesLoad_data() { QTest::addColumn<QString>("kind"); for(auto kind:{"calculator","clock","calendar","pdf"}) QTest::newRow(kind)<<QString(kind); }
    void pagesLoad() {
        QFETCH(QString,kind); CalculatorModel calculator; CalendarModel calendar; ClockModel clock; DocumentFiles documents; AppStyle style;
        QVERIFY(documents.open(QUrl::fromLocalFile(pdf)));
        QQmlApplicationEngine engine; engine.addImageProvider("theme",new ThemeIconProvider);
        QStringList warnings; connect(&engine,&QQmlEngine::warnings,this,[&](const QList<QQmlError> &errors){ for(const auto &e:errors) warnings.append(e.toString()); });
        auto *context=engine.rootContext(); context->setContextProperty("UiStyle",&style); context->setContextProperty("Calc",&calculator); context->setContextProperty("CalendarData",&calendar); context->setContextProperty("ClockData",&clock); context->setContextProperty("Documents",&documents); context->setContextProperty("AppKind",kind); context->setContextProperty("AppTitle",QString("Test"));
        engine.load(QUrl::fromLocalFile(QString(FIDAN_SOURCE_DIR)+"/src/apps/qml/Main.qml"));
        QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
        auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first()); QVERIFY(window);
        QTest::qWait(500);
        if(kind=="calculator") {
            auto *field=window->findChild<QQuickItem*>("calculatorExpression"); QVERIFY(field);
            field->forceActiveFocus();
            for(const auto key:{Qt::Key_2,Qt::Key_Plus,Qt::Key_3,Qt::Key_Asterisk,Qt::Key_4}) QTest::keyClick(window,key);
            QTest::keyClick(window,Qt::Key_Return);
            QCOMPARE(field->property("text").toString(),QString("14"));
        }
        if(kind=="clock") {
            auto *page=window->findChild<QObject*>("clockPage"); QVERIFY(page); page->setProperty("tab",2);
            clock.startTimer(1); QTRY_VERIFY_WITH_TIMEOUT(!clock.counting(),1500);
            QVERIFY(page->property("finished").toBool()); page->setProperty("tab",0);
        }
        if(kind=="calendar") {
            auto *page=window->findChild<QObject*>("calendarPage"); QVERIFY(page);
            QQmlExpression open(qmlContext(page),page,"editEvent(null)"); open.evaluate(); QVERIFY(!open.hasError());
            auto *field=window->findChild<QQuickItem*>("eventTitle"); QVERIFY(field); field->setProperty("text",QString("Günlük plan"));
            auto *dialog=window->findChild<QObject*>("eventEditor"); QVERIFY(dialog); QVERIFY(dialog->property("visible").toBool());
            QMetaObject::invokeMethod(dialog,"close");
        }
        QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
        QVERIFY(!window->grabWindow().isNull());
        if(kind=="pdf") { auto *document=window->findChild<QObject*>("pdfDocument"); QVERIFY(document); QTRY_COMPARE(document->property("pageCount").toInt(),2); }
        const QString capture=qEnvironmentVariable("FIDAN_UTILITY_SCREENSHOT_DIR");
        if(!capture.isEmpty()) {
            QDir().mkpath(capture); QVERIFY(window->grabWindow().save(capture+"/"+kind+"-dark.png"));
            QSettings settings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)+"/fidan/shell.conf",QSettings::IniFormat);
            settings.setValue("appearance/darkTheme",false); settings.sync(); QTest::qWait(1200);
            QVERIFY(window->grabWindow().save(capture+"/"+kind+"-light.png"));
            settings.setValue("appearance/darkTheme",true); settings.sync();
        }
        window->close();
    }
};
QTEST_MAIN(UtilitiesTest)
#include "tst_utilities.moc"
