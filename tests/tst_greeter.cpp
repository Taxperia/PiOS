// SPDX-License-Identifier: GPL-3.0-or-later
#include "../src/greeter/GreeterController.h"
#include <QtTest>
#include <QLocalServer>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QtEndian>
#include <memory>

class GreeterTest final : public QObject {
    Q_OBJECT
    QTemporaryDir directory;
    QLocalServer server;
    std::unique_ptr<GreeterController> controller;
    QLocalSocket *peer = nullptr;
    void reply(QJsonObject value) {
        const auto json=QJsonDocument(value).toJson(QJsonDocument::Compact);
        const quint32 length=qToLittleEndian(quint32(json.size()));
        peer->write(reinterpret_cast<const char *>(&length),4); peer->write(json); peer->flush();
    }
    QJsonObject request() {
        QElapsedTimer timer; timer.start();
        while(peer->bytesAvailable()<4 && timer.elapsed()<3000) QTest::qWait(5);
        if(peer->bytesAvailable()<4) return {};
        const auto header=peer->read(4); const auto size=qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(header.constData()));
        while(peer->bytesAvailable()<size && timer.elapsed()<3000) QTest::qWait(5);
        return QJsonDocument::fromJson(peer->read(size)).object();
    }
private slots:
    void initTestCase() { QQuickStyle::setStyle("Basic"); }
    void init() {
        QVERIFY(server.listen(directory.filePath(QUuid::createUuid().toString(QUuid::Id128))));
        qputenv("GREETD_SOCK",server.fullServerName().toUtf8()); controller=std::make_unique<GreeterController>();
        QTRY_VERIFY(server.hasPendingConnections()); peer=server.nextPendingConnection(); QVERIFY(peer);
    }
    void cleanup() { controller.reset(); delete peer; peer=nullptr; server.close(); qunsetenv("GREETD_SOCK"); }
    void nativeUiCompletesAuthenticationAndStartsExactlyOneSession() {
        QQmlApplicationEngine engine; engine.rootContext()->setContextProperty("GreeterBackend",controller.get());
        QStringList warnings; connect(&engine,&QQmlEngine::warnings,this,[&](const QList<QQmlError> &errors){for(const auto &error:errors)warnings.append(error.toString());});
        engine.load(QUrl::fromLocalFile(QString(FIDAN_SOURCE_DIR)+"/src/greeter/qml/Greeter.qml"));
        QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
        auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window); window->setProperty("signInVisible",true);
        auto *password=window->findChild<QObject *>("greeterPassword"); QVERIFY(password);
        QSignalSpy authenticated(controller.get(),&GreeterController::authenticated);
        controller->beginLogin("maker"); QCOMPARE(request().value("type").toString(),"create_session");
        reply({{"type","auth_message"},{"auth_message_type","secret"},{"auth_message","Password:"}});
        QTRY_COMPARE(controller->prompt(),"Password:"); QTRY_VERIFY(password->property("visible").toBool());
        controller->answer("fixture-password"); const auto answer=request(); QCOMPARE(answer.value("type").toString(),"post_auth_message_response"); QCOMPARE(answer.value("response").toString(),"fixture-password");
        reply({{"type","success"}}); const auto start=request(); QCOMPARE(start.value("type").toString(),"start_session"); QCOMPARE(start.value("cmd").toArray().first().toString(),"fidan-session");
        QCOMPARE(authenticated.count(),0); reply({{"type","success"}}); QTRY_COMPARE(authenticated.count(),1);
        QTest::qWait(30); QCOMPARE(peer->bytesAvailable(),0); QVERIFY(!controller->busy());
        QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n'))); window->close();
    }
    void failedPasswordCancelsAndAllowsRetry() {
        QSignalSpy authenticated(controller.get(),&GreeterController::authenticated);
        controller->beginLogin("maker"); request();
        reply({{"type","auth_message"},{"auth_message_type","secret"},{"auth_message","Password:"}});
        QTRY_VERIFY(!controller->busy()); controller->answer("wrong"); request();
        reply({{"type","error"},{"error_type","auth_error"},{"description","Bad credentials"}});
        QCOMPARE(request().value("type").toString(),"cancel_session"); reply({{"type","success"}});
        QTRY_VERIFY(!controller->busy()); QVERIFY(controller->prompt().isEmpty()); QCOMPARE(controller->error(),"Bad credentials"); QCOMPARE(authenticated.count(),0);
        controller->beginLogin("another"); QCOMPARE(request().value("username").toString(),"another");
    }
    void informationalAndVisiblePromptsFollowTheProtocol() {
        controller->beginLogin("maker"); request();
        reply({{"type","auth_message"},{"auth_message_type","info"},{"auth_message","Notice"}});
        const auto ack=request(); QCOMPARE(ack.value("type").toString(),"post_auth_message_response"); QVERIFY(!ack.contains("response"));
        reply({{"type","auth_message"},{"auth_message_type","visible"},{"auth_message","Code:"}});
        QTRY_VERIFY(!controller->busy()); QVERIFY(!controller->secretPrompt());
        controller->cancel(); QCOMPARE(request().value("type").toString(),"cancel_session"); reply({{"type","success"}});
        QTRY_VERIFY(!controller->busy()); QTest::qWait(30); QCOMPARE(peer->bytesAvailable(),0);
    }
};
QTEST_MAIN(GreeterTest)
#include "tst_greeter.moc"
